# EerieLeap Expression Engine

A small compiler and an allocation-free evaluator for the arithmetic expressions that EerieLeap
units attach to sensors (`x * 4 + 1.6`, `(raw >> 8) & 0xFF`, `temp > 90 ? 1 : 0`). An expression
is compiled once, at configuration time, into a few dozen bytes of bytecode; evaluating it then
costs no allocation, no exception and no lock, so it can run from a sampling thread every few
milliseconds.

The language is the one EerieLeap sensor configurations use: arithmetic, comparisons, short-circuit
logic, a ternary, two dozen math functions and named variables, plus bitwise operators and hex and
binary literals for decoding raw payloads.

- Plain C++23, standard library only. No Zephyr, no third-party code in the library itself.
- Builds with or without exceptions and RTTI; every failure is a `std::expected` error with the
  position of the offending text.
- One to three allocations per compiled expression, through the `std::pmr::memory_resource` the
  caller passes. Zero per evaluation.
- Compile-time caps for text length, code size, variables, stack, nesting and argument counts,
  tightened per call with a `Limits` argument.
- A Zephyr module out of the box (`zephyr/`), with Kconfig for the caps.
- Tested on the host with GoogleTest under AddressSanitizer and UndefinedBehaviorSanitizer, with a
  random-expression property test and a libFuzzer harness, and on Zephyr with a Twister suite on
  native_sim and QEMU ARM, AArch64, RISC-V and big-endian MIPS.

## Example

```cpp
#include <eerie_leap/expression_engine/expression_engine.hpp>

namespace ee = eerie_leap::expression_engine;

const auto program = ee::Compile("(raw >> 8) & 0xFF");
if(!program) {
    // program.error() is {code, position, length}; Describe(code) is "unknown function" etc.
    return;
}

// Variables are numbered in order of first appearance; VariableName(i) gives the name.
const std::array<ee::Value, 1> variables{4660.0f};   // raw
const ee::Value value = program->Evaluate(variables); // 18
```

`examples/expr_cli.cpp` does the same from the command line and prints the bytecode:

```
$ expr_cli "(raw >> 8) & 0xFF" raw=4660
  0 PushVar     raw
  1 PushConst   8
  2 Shr
  3 PushConst   255
  4 BitAnd
  5 End
= 18
```

## Building and testing on the host

Requires CMake 3.25, Ninja and GCC 13 or Clang 19 or newer (Clang 18 with libstdc++ 13 lacks `std::expected`). GoogleTest is fetched automatically
(`EXPRESSION_ENGINE_FETCH_GTEST=OFF` uses an installed one instead).

```sh
cmake --preset dev && cmake --build --preset dev && ctest --preset dev     # Debug, ASan + UBSan
cmake --preset release && cmake --build --preset release && ctest --preset release
cmake --preset no-exceptions && cmake --build --preset no-exceptions && ctest --preset no-exceptions
```

Other presets: `tidy` (clang-tidy) and `fuzz` (clang, builds `tests/compile_fuzzer`).

The committed `.clang-format` is the style; CI checks it with `clang-format --dry-run -Werror`.

## Using it from Zephyr

Add the repository to `EXTRA_ZEPHYR_MODULES` (or to the west manifest) and set

```
CONFIG_CPP=y
CONFIG_STD_CPP23=y
CONFIG_EXPRESSION_ENGINE=y
```

The Twister suite runs with `west twister -T tests/zephyr -p native_sim`. See
[docs/INTEGRATION.md](docs/INTEGRATION.md) for the details, the Kconfig caps and an adapter example.

## Documentation

- [docs/LANGUAGE.md](docs/LANGUAGE.md): the expression language, precedence, functions, numbers,
  bitwise semantics and every error code.
- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md): the compiler pipeline, the bytecode, the memory
  model, the invariants and how to extend it.
- [docs/INTEGRATION.md](docs/INTEGRATION.md): consuming the library from CMake and from Zephyr.
- [CHANGELOG.md](CHANGELOG.md).

## Layout

```
include/eerie_leap/expression_engine/   public headers
src/                                    lexer, parser, emitter, evaluator, functions
zephyr/                                 module.yml, Kconfig, CMake glue
tests/unit, tests/property, tests/fuzz  host tests
tests/vectors                           behaviour tables shared with the Zephyr suite
tests/zephyr                            Twister suite
examples/                               expr_cli
docs/
```

## License

Apache License 2.0, see [LICENSE](LICENSE).
