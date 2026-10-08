#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <memory_resource>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "error.hpp"
#include "instruction.hpp"
#include "limits.hpp"
#include "types.hpp"

namespace eerie_leap::expression_engine {

class Program;

[[nodiscard]] std::expected<Program, CompileError> Compile(
    std::string_view text,
    std::pmr::memory_resource* resource,
    const Limits& limits
) noexcept;

// A compiled expression: bytecode plus the names of the variables it reads, in index order.
//
// Only Compile() constructs a Program. That is the invariant Evaluate() relies on: every jump
// target is in range, the stack never exceeds MaxStackDepth() <= limits::kMaxStack, every PushVar
// index is below VariableCount() and every Call has a valid function and argument count, so the
// evaluation loop has no bounds checks.
//
// A Program owns everything it refers to (the names are copied into symbols_), is immutable once
// built and allocation-free to evaluate, so one Program can be evaluated from several threads at
// once.
class Program {
public:
    using allocator_type = std::pmr::polymorphic_allocator<>;

    Program(Program&&) noexcept = default;
    Program& operator=(Program&&) = default;
    Program(const Program&) = delete;
    Program& operator=(const Program&) = delete;
    ~Program() = default;

    // variables[i] is the value of VariableName(i). Returns NaN when fewer values than
    // VariableCount() are given; that check is the only one in the evaluator.
    [[nodiscard]] Value Evaluate(std::span<const Value> variables) const noexcept;

    [[nodiscard]] std::span<const Instruction> Code() const noexcept {
        return code_;
    }

    [[nodiscard]] std::size_t VariableCount() const noexcept {
        return names_.size();
    }

    // index < VariableCount()
    [[nodiscard]] std::string_view VariableName(std::size_t index) const noexcept {
        const NameRef& ref = names_[index];
        return std::string_view{symbols_}.substr(ref.offset, ref.length);
    }

    // The names in index order, as a range of std::string_view.
    [[nodiscard]] auto VariableNames() const noexcept {
        return std::views::iota(std::size_t{0}, names_.size()) |
               std::views::transform([this](std::size_t index) { return VariableName(index); });
    }

    [[nodiscard]] std::optional<std::size_t> VariableIndex(std::string_view name) const noexcept;

    // Upper bound of the evaluation stack this program uses; at most limits::kMaxStack.
    [[nodiscard]] std::uint8_t MaxStackDepth() const noexcept {
        return max_stack_;
    }

    [[nodiscard]] allocator_type get_allocator() const noexcept {
        return code_.get_allocator();
    }

private:
    friend std::expected<Program, CompileError> Compile(
        std::string_view text,
        std::pmr::memory_resource* resource,
        const Limits& limits
    ) noexcept;

    struct NameRef {
        std::uint16_t offset; // into symbols_
        std::uint16_t length;
    };

    explicit Program(allocator_type alloc) : code_(alloc), names_(alloc), symbols_(alloc) {}

    std::pmr::vector<Instruction> code_; // exact size, ends with Op::End
    std::pmr::vector<NameRef> names_;    // one per variable index
    std::pmr::string symbols_;           // the names, concatenated
    std::uint8_t max_stack_ = 0;
};

} // namespace eerie_leap::expression_engine
