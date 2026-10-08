// libFuzzer harness: Compile() on arbitrary bytes must never crash, hang or trip a sanitizer, and a
// program it accepts must evaluate and disassemble.
//
//   cmake --preset fuzz && cmake --build --preset fuzz
//   build/fuzz/tests/compile_fuzzer -max_total_time=60 corpus/

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>

#include <eerie_leap/expression_engine/expression_engine.hpp>

namespace ee = eerie_leap::expression_engine;

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    const std::string_view text{reinterpret_cast<const char*>(data), size};

    const auto program = ee::Compile(text);
    if(!program) {
        (void)ee::Describe(program.error().code);
        return 0;
    }

    std::array<ee::Value, ee::limits::kMaxVariables> variables{};
    for(std::size_t i = 0; i < variables.size() && i * sizeof(float) + sizeof(float) <= size; ++i) {
        std::memcpy(&variables[i], data + i * sizeof(float), sizeof(float));
    }
    (void)program->Evaluate(variables);

    ee::Disassemble(*program, [](std::string_view) {});
    return 0;
}
