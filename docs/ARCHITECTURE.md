# Architecture

```
text ──▶ Lexer ──▶ Parser (precedence climbing) ──▶ Emitter (scratch, folding, depth) ──▶ Program ──▶ Evaluate(variables)
                      │                                   │
                      └──────────── CompileError ◀────────┘
```

Each stage is one class with one responsibility and its own test file. The lexer and the parser
know nothing about the bytecode, the emitter knows nothing about syntax, and the evaluator knows
nothing about text. `Compile()` in `src/compiler.cpp` is the only function that strings them
together.

| Part | Files | Responsibility |
| --- | --- | --- |
| Public API | `include/eerie_leap/expression_engine/*.hpp` | `Compile`, `Program`, `Limits`, `CompileError`, the function table, `Disassemble` |
| Lexer | `src/lexer.{hpp,cpp}` | text to tokens, on demand, no buffer |
| Parser | `src/parser.{hpp,cpp}` | grammar and precedence; drives the emitter |
| Emitter | `src/emitter.{hpp,cpp}` | bytecode in a scratch buffer, constant folding, stack depth, jump labels, variable interning |
| Semantics | `src/ops.hpp`, `src/bit_ops.hpp`, `src/functions.cpp` | what every opcode and function computes; shared by the evaluator and the folder |
| Evaluator | `src/program.cpp` | the `switch` loop |
| Disassembler | `src/disassembler.cpp` | one text line per instruction, allocation-free |

## Public API

- `Compile(text, resource, limits) -> std::expected<Program, CompileError>` is `noexcept` and is
  the only way to obtain a `Program`.
- `Program::Evaluate(std::span<const Value>) -> Value`, `noexcept`, allocation-free, reentrant.
  Returns NaN when fewer values than `VariableCount()` are given; that is the only check it makes.
- `Program::VariableName(i)`, `VariableNames()`, `VariableIndex(name)`, `VariableCount()`,
  `Code()`, `MaxStackDepth()`, `get_allocator()`.
- `Limits` is an aggregate whose fields default to the compile-time caps in `limits.hpp` and are
  clamped to them (`Limits::Clamped()`), so a caller can only tighten.
- `kFunctions` / `FindFunction` / `Spec` / `Invoke` in `functions.hpp` expose the function table.
- `Disassemble(program, sink)` calls the sink once per instruction with a `std::string_view` that
  lives in a stack buffer.

Only standard headers are used: `<expected>`, `<span>`, `<string_view>`, `<charconv>`,
`<memory_resource>`, `<cmath>`, `<ranges>`, `<bit>`. The library builds with `-fno-exceptions
-fno-rtti` (the `no-exceptions` preset and the Zephyr suite prove it).

## Bytecode

A `Program` is a vector of eight-byte, trivially copyable instructions ending in `End`, plus a
table of variable names:

```
struct Instruction { Op op; std::uint8_t aux; std::uint16_t arg; Value imm; };
```

| Opcode | `aux` / `arg` / `imm` | Stack effect |
| --- | --- | --- |
| `PushConst` | `imm` = the value | +1 |
| `PushVar` | `arg` = variable index | +1 |
| `Neg`, `BitNot`, `ToBool` | | 0 |
| `Add Sub Mul Div Pow Lt Gt Le Ge Eq Ne BitAnd BitOr Shl Shr` | | -1 |
| `AndJump` | `arg` = target | pops 1; on the taken path pushes `0` |
| `OrJump` | `arg` = target | pops 1; on the taken path pushes `1` |
| `JumpIfFalse` | `arg` = target | -1 |
| `Jump` | `arg` = target | 0 |
| `Call` | `aux` = `FunctionId`, `arg` = argument count | 1 - argc |
| `End` | | returns the one remaining value |

Jump targets are absolute instruction indices. Control flow is encoded as:

```
a && b      <a>  AndJump L    <b>  ToBool   L:
a || b      <a>  OrJump  L    <b>  ToBool   L:
c ? a : b   <c>  JumpIfFalse L1  <a>  Jump L2  L1: <b>  L2:
```

Both paths of every jump leave the same stack depth, by construction in the emitter.

`src/ops.hpp` (`ApplyBinary`, `ApplyUnary`) and `src/functions.cpp` (`Invoke`) are the single
definition of what each opcode computes. The evaluator and the constant folder both call them, so
a constant folded at compile time has exactly the bits the same expression would produce at run
time.

## The emitter and constant folding

The emitter writes into a fixed `std::array<Instruction, kMaxInstructions>` on the stack. It keeps
an abstract **operand stack** beside the real one: an entry is either *materialized* (a value the
code will have produced at that point) or a *pending constant* that has not been written to the
code yet.

- `EmitConst` only pushes a pending constant.
- An operator or call whose operands are all pending constants is computed right away and
  replaced by one pending constant. Nothing is written.
- Anything that needs its operands in the code (`PushVar`, a non-constant operator, a jump, a
  label, a call with a non-constant argument, `End`) first **flushes** the pending constants, in
  order, as `PushConst` instructions.

Invariant: the pending constants are always a suffix of the operand stack, because everything that
materializes a value flushes them first. Consequences:

- `2 * _pi` and `sum(1, 2, 3)` are one `PushConst`; `1 + 2 * x` is `PushConst 1, PushConst 2,
  PushVar x, Mul, Add` (the two constants cannot fold because `2 * x` is not constant).
- The reported `MaxStackDepth()` counts materialized values only, so it is exact: `2 * 3` has
  depth 1.
- A constant can never be folded across a jump target, because placing a label flushes. In
  `(0 ? 2 : 3) + 4` the `3` at the false label and the `4` stay separate instructions
  (`tests/unit/emitter_test.cpp`, `DoesNotFoldAcrossAJumpTarget`).

Jumps carry a `JumpRef{index, arrival_depth, arrival_operands}`; `PatchJump` sets the target to
the next instruction and resets both stacks to the state the target sees. The parser is the only
caller, and it only ever emits the three shapes above.

Variable names are interned as `(offset, length)` slices of the source text while compiling and
copied into the `Program`'s own `symbols_` string when it is built, so a `Program` never refers to
the text it was compiled from.

## Parser

Precedence climbing with one `constexpr` table of the binary operators (`src/parser.cpp`,
`kBinaryOperators`). `^` is parsed in `ParsePower`, after `ParsePrimary`, because it binds tighter
than the unary operators and is right-associative; its exponent is parsed with `ParseUnary` so that
`2^-1` and `2^3^2` work without parentheses.

Recursion is bounded by `Limits::max_nesting`: `Enter()` is called at every construct that
nests, which is `(`, a call, `?:`, a unary operator and the exponent of `^`. Precedence climbing
itself recurses at most once per precedence level (eight) between two nesting levels.

Every error carries the token that caused it. Emitter failures (which only know a code) are
lifted to a `CompileError` with the position of the operator, literal or name being compiled.

## Memory

| | Bytes |
| --- | ---: |
| Per instruction | 8 |
| Per variable | 4 (`NameRef`) + the name's characters |
| `Program` object | three pmr containers plus the depth: 56 on a 32-bit target, 112 on x86-64 |
| `x * 4 + 1.6` | 6 instructions = 48 B of code, 4 B of names, `x` in the small-string buffer; 2 allocations |
| `sensor_1 + sensor_2` | 32 B of code, 8 B of names, 16 B of symbols (beyond the buffer); 3 allocations |

`Compile()` allocates one to three times, through the resource it is given and only there (the
allocation test counts every call of the global `operator new` while compiling); a failed
compilation allocates nothing. `Evaluate()` and `Disassemble()` never allocate.

### Stack

Measured with `-fstack-usage` on x86-64 at `-O2`:

| Function | Frame |
| --- | ---: |
| `Compile` (scratch code buffer, operand stack, lexer, parser) | 1,328 B |
| `ParseCall` | 192 B |
| `ParseBinary` | 128 B |
| `ParseTernary`, `ParsePrimary` | 96 B each |
| `ParseUnary`, `ParsePower` | 80 B each |
| `Evaluate` | 144 B plus the 64 B value stack |

One nesting level (a parenthesis, call, ternary, unary or exponent) costs the chain
`ParsePrimary → ParseTernary → ParseBinary → ParseUnary → ParsePower`, about 0.6 kB, plus 128 B for
each further precedence level climbed inside it (up to 1.5 kB). At the default `max_nesting` of
16 the worst case is around 25 kB; typical sensor expressions nest two or three levels deep and
need under 4 kB. A caller on a small stack lowers `max_nesting` through `Limits` or Kconfig.

## Invariants the evaluator relies on

Only `Compile()` can construct a `Program` (private constructor, `Compile` is a friend). Because
every program went through the emitter:

- every jump target is a valid instruction index,
- the stack never exceeds `MaxStackDepth()`, which is at most `limits::kMaxStack`, the size of the
  evaluator's local stack,
- every `PushVar` index is below `VariableCount()`,
- every `Call` names a real `FunctionId` with an argument count inside its range,
- the last instruction is `End` and exactly one value is on the stack when it runs.

The evaluation loop therefore has no bounds checks; its one check is the size of the variables
span, because that comes from the caller.

## Thread safety

A `Program` is immutable after `Compile()`. `Evaluate()` is `const`, uses only its own stack frame
and takes no lock, so one program can be evaluated from several threads at once (there is a test
for it). `Compile()` has no shared state either. The function table and the error tables are
`constexpr`.

## Extending

**A function.** Add the enumerator to `FunctionId` and the row to `kFunctions`, both in
alphabetical order (the `static_assert`s check it), add its case to `Invoke()` in
`src/functions.cpp`, and add rows to `tests/vectors/function_vectors.hpp` (the test that every
function has a vector fails until you do). The parser, the folder, the disassembler and the docs
table pick it up from the table.

**A binary operator.** Add the token to `TokenKind` and the lexer's `switch`, the opcode to `Op`
(append; the disassembler's name table and `IsBinary()` in `src/ops.hpp` must follow), its
semantics to `ApplyBinary()`, and the row to `kBinaryOperators` in `src/parser.cpp` with its
precedence. Add it to `kOperators` in `tests/support/expression_generator.hpp` so the property
test covers it, and to the precedence table in `docs/LANGUAGE.md`.

**An error code.** Append to `CompileErrorCode`, bump `kCompileErrorCodeCount`, add the
description and the name in `src/error.cpp`, and a row in `tests/vectors/error_vectors.hpp`.

**A cap.** Add it to `cmake/sources.cmake` (`EXPRESSION_ENGINE_CAPS` and its default), to
`limits.hpp` (macro default, constant, `static_assert`, `Limits` field and `Clamped()`), and to
`zephyr/Kconfig`.

## Tests

| Suite | Proves |
| --- | --- |
| `tests/unit/lexer_test.cpp` | every token with position and length, number forms, integer literals and their errors |
| `tests/unit/emitter_test.cpp` | folding rules, flush order, interning, jump patching, depth accounting, each limit |
| `tests/unit/parser_test.cpp` | bytecode snapshots and a precedence table of minimal versus parenthesized forms |
| `tests/unit/compiler_test.cpp` | every error vector, each cap at the cap and one past it, lowered `Limits`, clamping, out of memory, ownership of names |
| `tests/unit/evaluator_test.cpp` | the evaluation vectors, `ToInt32` edge cases, shifts, NaN truth, reentrancy |
| `tests/unit/functions_test.cpp` | each function folded and at run time, the documented formulas, argument-count boundaries |
| `tests/unit/allocation_test.cpp` | 1 to 3 allocations per compile, only through the resource; zero per evaluation |
| `tests/unit/disassembler_test.cpp` | every operand kind and every opcode name |
| `tests/property/generator_test.cpp` | 6,000 random trees rendered with full and minimal parentheses evaluate to the generated value, bit for bit |
| `tests/fuzz/compile_fuzzer.cpp` | libFuzzer harness (clang) |
| `tests/zephyr` | the shared vector tables on native_sim and four QEMU targets, without exceptions and RTTI |

The vector tables in `tests/vectors/` are the behaviour specification shared by the host and
Zephyr suites; a new behaviour goes there first.
