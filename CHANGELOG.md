# Changelog

## 0.1.0 (unreleased)

First version, designed in the EerieLeap gauge's expression engine plan.

- Language: `+ - * / ^`, comparisons, short-circuit `&& ||`, `?:`, 27 functions, `_pi`, `_e`,
  bitwise `& | << >> ~`, `xor(a, b)`, hex and binary integer literals.
- `Compile()` returns `std::expected<Program, CompileError>`; the library builds without
  exceptions and RTTI.
- Deferred-constant emitter: constant subexpressions fold into one `PushConst`, the reported stack
  depth is exact, and nothing folds across a jump target.
- Compile-time caps (`EXPRESSION_ENGINE_MAX_*`, Kconfig on Zephyr) and a per-call `Limits`
  argument.
- GoogleTest suite with sanitizers, random-expression property test, libFuzzer harness, Twister
  suite on native_sim, qemu_cortex_a9, qemu_cortex_a53, qemu_riscv64 and qemu_malta.
