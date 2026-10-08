# Integration

## From CMake on the host

The library is the target `eerie_leap::expression_engine`. Any of these work:

```cmake
# As a subdirectory (a Git submodule, for instance).
add_subdirectory(third_party/eerie_leap_expression_engine)
target_link_libraries(my_app PRIVATE eerie_leap::expression_engine)

# With FetchContent.
include(FetchContent)
FetchContent_Declare(eerie_leap_expression_engine
    GIT_REPOSITORY <url>
    GIT_TAG v0.1.0)
FetchContent_MakeAvailable(eerie_leap_expression_engine)
target_link_libraries(my_app PRIVATE eerie_leap::expression_engine)

# After `cmake --install`.
find_package(eerie_leap_expression_engine CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE eerie_leap::expression_engine)
```

Tests and examples are built only when the repository is the top-level project
(`EXPRESSION_ENGINE_BUILD_TESTS`, `EXPRESSION_ENGINE_BUILD_EXAMPLES`). The library carries no
warning flags into consumers; it needs `cxx_std_23`.

### Caps

The six compile-time caps are CMake cache entries with these defaults:

| Cache entry | Default | What it bounds |
| --- | ---: | --- |
| `EXPRESSION_ENGINE_MAX_LENGTH` | 256 | characters of text |
| `EXPRESSION_ENGINE_MAX_INSTRUCTIONS` | 64 | instructions, 8 bytes each, including `End` |
| `EXPRESSION_ENGINE_MAX_VARIABLES` | 16 | distinct variable names |
| `EXPRESSION_ENGINE_MAX_STACK` | 16 | evaluation stack slots, 4 bytes each |
| `EXPRESSION_ENGINE_MAX_NESTING` | 16 | parser recursion depth |
| `EXPRESSION_ENGINE_MAX_ARGUMENTS` | 8 | arguments of `sum`, `avg`, `min`, `max` |

They become `EXPRESSION_ENGINE_MAX_*` macros on the target's public interface, so a consumer's
translation units see the same values as the library. Every cap can be lowered per call through
the `Limits` argument of `Compile()`; it cannot be raised.

## From Zephyr

The repository is a Zephyr module (`zephyr/module.yml`). Register it either in the application's
CMake before `find_package(Zephyr)`:

```cmake
set(EXTRA_ZEPHYR_MODULES "${CMAKE_CURRENT_SOURCE_DIR}/../modules/eerie_leap_expression_engine")
```

or as a project in the west manifest. Then in `prj.conf`:

```
CONFIG_CPP=y
CONFIG_STD_CPP23=y
CONFIG_EXPRESSION_ENGINE=y
```

`CONFIG_EXPRESSION_ENGINE` selects `REQUIRES_FULL_LIBCPP`; the engine itself needs neither
`CONFIG_CPP_EXCEPTIONS` nor `CONFIG_CPP_RTTI`. The caps are the Kconfig options
`CONFIG_EXPRESSION_ENGINE_MAX_*` with the same names and defaults as the CMake cache entries;
`zephyr/CMakeLists.txt` turns them into the macros with `zephyr_compile_definitions()`, globally,
so every translation unit agrees on them.

Stack: `Compile()` runs on the calling thread's stack. Measured on x86-64 at `-O2`, its own frame
is 1.3 kB (the scratch code buffer, the operand stack and the lexer and parser objects) and each
nesting level of the expression adds between 0.6 kB (one parenthesis) and 1.5 kB (a chain through
every precedence level). With the default `max_nesting` of 16 the worst case is therefore around
25 kB; 32-bit targets use less. A thread with a small stack should compile with
`Limits{.max_nesting = 8}` or lower the Kconfig cap. `Evaluate()` needs about 150 bytes plus the
`kMaxStack * 4` byte value stack.

The Twister suite in `tests/zephyr` runs from any workspace that has this module:

```sh
west twister -c --disable-warnings-as-errors -O ./twister-out -T tests/zephyr -p native_sim
```

and on `qemu_cortex_a9`, `qemu_cortex_a53`, `qemu_riscv64` and `qemu_malta`. The repository is
also a west manifest (`west.yml`), so the suite can run from a clone of this repository alone.

## Memory

`Compile()` allocates from the `std::pmr::memory_resource*` it is given (the default resource when
none is): once for the code, once for the variable name table when the expression has variables,
and once more for the names' characters when they exceed the small-string buffer. The resulting
`Program` is allocator-aware (`get_allocator()`), movable, not copyable, and frees through the same
resource. Nothing else in the library allocates. `Evaluate()` and `Disassemble()` are
allocation-free.

A caller that keeps many programs can hand each `Compile()` the same arena, for example a
`std::pmr::monotonic_buffer_resource` that is released as a whole when the configuration changes:

```cpp
std::pmr::monotonic_buffer_resource arena(16 * 1024);
std::vector<ee::Program> programs;
for(const std::string_view text : expressions) {
    auto program = ee::Compile(text, &arena);
    if(program) {
        programs.push_back(std::move(*program));
    }
}
```

When exceptions are enabled, an allocation failure inside `Compile()` becomes
`CompileErrorCode::OutOfMemory`. Without exceptions, an allocation failure follows the resource's
own policy (libstdc++'s default resource terminates).

## Errors

`CompileError` is `{code, position, length}`: a byte offset into the text and the length of the
offending token (0 at the end of the input). `Describe(code)` is a short message, `Name(code)` the
enumerator. A caller that reports errors to a user can show the text with a caret at `position`,
as `examples/expr_cli.cpp` does:

```
error: unbalanced parenthesis at 4
2 * (x
    ^
```

## Binding variables

A `Program` numbers the variables in order of first appearance and never interprets a name. The
caller decides which names mean what. The adapter for the EerieLeap sensor pipeline, where `x` is
the sensor's own input and every other name is a sensor ID, looks like this:

```cpp
class ExpressionEvaluator {
public:
    using allocator_type = std::pmr::polymorphic_allocator<>;

    static std::expected<ExpressionEvaluator, ee::CompileError> Create(
        allocator_type alloc,
        std::string_view expression
    ) noexcept {
        auto program = ee::Compile(expression, alloc.resource());
        if(!program) {
            return std::unexpected(program.error());
        }
        return ExpressionEvaluator(alloc, expression, std::move(*program));
    }

    // Sensor IDs the expression reads, in the order Evaluate() expects them (x excluded).
    auto GetVariableNames() const noexcept {
        return program_.VariableNames() | std::views::filter([](std::string_view name) { return name != "x"; });
    }

    bool UsesInput() const noexcept {
        return input_index_.has_value();
    }

    float Evaluate(std::span<const float> sensor_values, float input) const noexcept {
        std::array<float, ee::limits::kMaxVariables> values{};
        std::size_t next = 0;
        for(std::size_t i = 0; i < program_.VariableCount(); ++i) {
            values[i] = (input_index_ == i) ? input : sensor_values[next++];
        }
        return program_.Evaluate(std::span<const float>{values.data(), program_.VariableCount()});
    }

private:
    ExpressionEvaluator(allocator_type alloc, std::string_view expression, ee::Program program)
        : expression_(expression, alloc), program_(std::move(program)),
          input_index_(program_.VariableIndex("x")) {}

    std::pmr::string expression_; // the persisted text
    ee::Program program_;
    std::optional<std::size_t> input_index_;
};
```

Validation can compile the text once to check it (and to learn the variable names) and discard
the program; compiling is cheap and allocates only the program.
