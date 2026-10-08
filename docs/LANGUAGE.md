# The expression language

An expression computes one `float` from named variables: arithmetic, comparisons, logic, a
ternary, function calls, bitwise operators and integer literals. The formulas below are part of
the language, so an expression stored in a configuration keeps its value from one release to the
next.

## Grammar

```
expression  = ternary ;
ternary     = binary [ "?" ternary ":" ternary ] ;
binary      = unary { binary-op unary } ;                 (* by precedence, see below *)
unary       = ( "-" | "+" | "~" ) unary | power ;
power       = primary [ "^" unary ] ;                     (* right-associative *)
primary     = number | identifier | identifier "(" [ ternary { "," ternary } ] ")" | "(" ternary ")" ;
binary-op   = "||" | "&&" | "<" | ">" | "<=" | ">=" | "==" | "!=" | "|" | "&" | "<<" | ">>" | "+" | "-" | "*" | "/" ;
identifier  = ( letter | "_" ) { letter | digit | "_" } ;
number      = decimal | "0x" hex-digit { hex-digit } | "0b" bin-digit { bin-digit } ;
```

Whitespace (space, tab, CR, LF, VT, FF) separates tokens and is otherwise ignored.

## Precedence

Lowest first. The bitwise operators (levels 5 to 7) sit where Lua puts them: tighter than the
comparisons, looser than arithmetic.

| Level | Operators | Associativity |
| ---: | --- | --- |
| 1 | `?:` | right |
| 2 | `\|\|` | left |
| 3 | `&&` | left |
| 4 | `< > <= >= == !=` | left, non-chaining: `a < b < c` is `(a < b) < c` |
| 5 | `\|` | left |
| 6 | `&` | left |
| 7 | `<< >>` | left |
| 8 | `+ -` | left |
| 9 | `* /` | left |
| 10 | unary `- + ~` | prefix; `-x^2` is `-(x^2)` |
| 11 | `^` | right: `2^3^2` is `2^9`; the exponent may carry a sign: `2^-1` |

Bitwise operators bind tighter than comparisons and looser than arithmetic, so `x & 4 == 4` is
`(x & 4) == 4` (in C it would be `x & (4 == 4)`), `x + 1 & 0xFF` is `(x + 1) & 0xFF`, and
`x >> 8 & 0xFF` is `(x >> 8) & 0xFF`.

## Values and semantics

- Every value is a `float`. Arithmetic is IEEE: `1 / 0` is `inf`, `0 / 0` is NaN, NaN propagates,
  nothing traps.
- `^` is `powf`.
- Comparisons yield `1` or `0`. Any comparison with NaN yields `0`, including `==`; `!=` with NaN
  yields `1`.
- A value is **true** when it is not equal to `0`. NaN is therefore true.
- `a && b` is `0` when `a` is false, otherwise `1` if `b` is true and `0` if not. `a || b` is `1`
  when `a` is true, otherwise `1` if `b` is true and `0` if not. The right operand is not evaluated
  when the left one decides. Since operands have no side effects and cannot fail, the result is the
  same as evaluating both sides for every input, NaN included.
- `c ? a : b` evaluates only the chosen branch.
- Variables are any identifier that is not a function call or a constant. The engine assigns them
  indices in order of first appearance and never interprets their names; the caller binds them.

## Numbers

| Form | Examples | Notes |
| --- | --- | --- |
| Decimal | `1`, `1.`, `.5`, `1e3`, `2.5E-2` | parsed by `std::from_chars`; no sign (that is the unary operator), no `inf`/`nan` |
| Hexadecimal | `0xFF`, `0Xff` | 32-bit pattern read as a signed integer, see [Integer literals](#integer-literals) |
| Binary | `0b1010`, `0B11` | same |

A letter directly after a number is the start of the next token, so `1e` is `1` followed by the
identifier `e` (an error, since two values cannot follow each other) and `12ab` is `12` then `ab`.

Constants: `_pi` and `_e`, folded into the program.

## Functions

| Name | Arguments | Result |
| --- | --- | --- |
| `sin cos tan` | 1 | `<cmath>` |
| `asin acos atan` | 1 | `<cmath>` |
| `sinh cosh tanh` | 1 | `<cmath>` |
| `asinh acosh atanh` | 1 | `<cmath>` |
| `atan2` | 2 | `atan2(y, x)` |
| `exp` | 1 | `<cmath>` |
| `log`, `ln` | 1 | natural logarithm, under both names |
| `log10` | 1 | `<cmath>` |
| `log2` | 1 | computed as `log(v) / log(2)` |
| `sqrt` | 1 | `<cmath>` |
| `abs` | 1 | `fabs` |
| `sign` | 1 | `-1` for negative, `1` for positive, `0` for zero and NaN |
| `rint` | 1 | `floor(v + 0.5)`, so halves round up: `rint(2.5)` is `3`, `rint(-2.5)` is `-2` |
| `sum` | 1 to `max_arguments` | sum in argument order |
| `avg` | 1 to `max_arguments` | `sum / count` |
| `min`, `max` | 1 to `max_arguments` | folded with `std::min` / `std::max` from the first argument; a leading NaN wins, a later one loses |
| `xor` | 2 | bitwise exclusive or, see below |

A call needs the parenthesis directly after the name; `sin + 1` treats `sin` as a variable.
`rnd` is not provided.

## Bitwise operations

| Syntax | Meaning |
| --- | --- |
| `a & b`, `a \| b` | and, or |
| `xor(a, b)` | exclusive or (`^` is power) |
| `~a` | not |
| `a << n`, `a >> n` | shift left, arithmetic shift right |

Each operation converts its operands to 32-bit integers with JavaScript's `ToInt32`, operates, and
converts the result back to `float`:

- NaN and ±inf become 0.
- The fraction is dropped (truncation toward zero): `3.7 & 1` is `3 & 1` = 1; `-3.7` becomes `-3`.
- Values outside the int32 range wrap modulo 2^32.
- Results are signed two's complement: `~0` is `-1`, `1 << 31` is `-2147483648`.
- `>>` keeps the sign: `-8 >> 1` is `-4`.
- A shift count outside `0..31` gives 0, in both directions. The count is converted like any
  operand, so `1 << 2.9` is `1 << 2`.

**Precision.** A `float` holds 24 significant bits, so bit patterns are exact up to 2^24 and a
result with more significant bits is rounded when it is converted back. A decimal literal above
2^24 may already be rounded before the operation: `4294967295` is the float 2^32, which converts to
0. Write masks in hex, which is checked.

### Integer literals

A hex or binary literal denotes a 32-bit pattern interpreted as a signed integer: `0xFFFFFFFF` is
`-1`, `0xFFFF0000` is `-65536`, `0x80000000` is `-2147483648`, all exact in `float`. The literal
must survive the round trip through `float` and `ToInt32` unchanged; `0x1FFFFFF` (25 significant
bits) is rejected at compile time with `IntegerLiteralNotExact` rather than masking the wrong bits
at run time. More than 32 bits (`0x100000000`) and a prefix without digits (`0x`) are
`InvalidNumber`. Decimal literals are not checked: they are floats by nature.

## Errors

`Compile()` returns a `CompileError{code, position, length}` where `position` is the byte offset
of the offending text and `length` its size (0 at the end of the input).

| Code | When | Example | Position |
| --- | --- | --- | --- |
| `Empty` | nothing but whitespace | `""` | 0 |
| `TooLong` | longer than `max_length` | 257 characters | 256, the first excess character |
| `UnexpectedCharacter` | a character that starts no token: `! % $ "` quotes, non-ASCII | `a % b` | the character |
| `InvalidNumber` | `std::from_chars` rejects it, or an integer literal overflows or has no digits | `.`, `1e400`, `0x`, `0x100000000` | the number |
| `UnexpectedToken` | a token where another was expected | `1 2`, `1 + * 2`, `()` | the token |
| `UnexpectedEnd` | the text ends inside an expression | `1 +`, `x ? 1` | end of text, length 0 |
| `UnbalancedParenthesis` | `(` without `)` or `)` without `(` | `2 * (x`, `2 * x)`, `sin(1` | the parenthesis |
| `UnknownFunction` | a name followed by `(` that is not a function | `rnd(1)` | the name |
| `WrongArgumentCount` | outside the function's range or above `max_arguments` | `sin()`, `atan2(1)` | the name |
| `AssignmentNotSupported` | `=` | `x = 1` | the `=` |
| `IntegerLiteralNotExact` | see above | `0x1FFFFFF` | the literal |
| `TooManyInstructions` | above `max_instructions` | | the operator being compiled |
| `TooManyVariables` | above `max_variables` distinct names | | the variable |
| `StackTooDeep` | the evaluation stack would exceed `max_stack` | `x+(x+(x+...` | the operand |
| `NestingTooDeep` | parser recursion above `max_nesting` | 17 nested `(` | the `(`, call, `?`, unary or `^` |
| `OutOfMemory` | the memory resource failed (exceptions enabled only) | | 0 |

## Limits

| Limit | Default | Bounds |
| --- | ---: | --- |
| `max_length` | 256 | characters of text |
| `max_instructions` | 64 | bytecode size, 8 bytes each, including `End` |
| `max_variables` | 16 | distinct names |
| `max_stack` | 16 | evaluation stack slots |
| `max_nesting` | 16 | parser recursion: `(`, calls, `?:`, unary operators, `^` exponents |
| `max_arguments` | 8 | arguments of `sum`, `avg`, `min`, `max` |

The defaults are the build's caps (`EXPRESSION_ENGINE_MAX_*`, Kconfig on Zephyr). A `Limits`
argument to `Compile()` lowers any of them for one call; values above a cap are clamped to it.
