#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <functional>
#include <span>
#include <string_view>

#include "program.hpp"

namespace eerie_leap::expression_engine {

namespace detail {

inline constexpr std::size_t kDisassemblyLineCapacity = 96;

// Writes "<index> <opcode> <operand>" for one instruction into `buffer` and returns the part written.
[[nodiscard]] std::string_view FormatInstruction(
    const Program& program,
    std::size_t index,
    std::span<char> buffer
) noexcept;

} // namespace detail

// Calls `sink` once per instruction with a line such as "  3 Call        sum/2". No allocation;
// each line lives in a stack buffer until the sink returns.
template <std::invocable<std::string_view> Sink>
void Disassemble(const Program& program, Sink&& sink) {
    std::array<char, detail::kDisassemblyLineCapacity> buffer{};
    const std::size_t count = program.Code().size();
    for(std::size_t index = 0; index < count; ++index) {
        std::invoke(sink, detail::FormatInstruction(program, index, buffer));
    }
}

} // namespace eerie_leap::expression_engine
