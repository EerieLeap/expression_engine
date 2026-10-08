#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <string_view>

#include "eerie_leap/expression_engine/error.hpp"
#include "eerie_leap/expression_engine/functions.hpp"
#include "eerie_leap/expression_engine/instruction.hpp"
#include "eerie_leap/expression_engine/limits.hpp"
#include "eerie_leap/expression_engine/types.hpp"

namespace eerie_leap::expression_engine::detail {

// A variable name as a slice of the source text.
struct NameRef {
    std::uint16_t offset;
    std::uint16_t length;
};

// A jump whose target is patched later, with the stack state the target must see.
struct JumpRef {
    std::uint16_t index;           // of the jump instruction
    std::uint8_t arrival_depth;    // materialized stack depth at the target
    std::uint8_t arrival_operands; // abstract operand count at the target
};

// Builds the bytecode in a fixed scratch buffer while the parser walks the expression.
//
// Constants are not written to the code when they are pushed; they stay pending on an abstract
// operand stack until something needs them in the code (a variable, a jump, a label, a call with
// a non-constant argument, the end). An operator whose operands are all pending constants is
// computed right away and replaces them with one pending constant. This folds "2 * _pi" into one
// PushConst, keeps the reported stack depth exact, and cannot fold across a jump target, since
// every label flushes the pending constants first.
//
// Invariant: the pending constants are always a suffix of the operand stack, because anything
// that materializes a value flushes them first.
class Emitter {
public:
    using Result = std::expected<void, CompileErrorCode>;

    Emitter(std::string_view text, const Limits& limits) noexcept;

    [[nodiscard]] Result EmitConst(Value value) noexcept;
    [[nodiscard]] Result EmitVariable(std::string_view name) noexcept; // a slice of the text
    [[nodiscard]] Result EmitUnary(Op op) noexcept;                    // Neg, BitNot, ToBool
    [[nodiscard]] Result EmitBinary(Op op) noexcept;
    [[nodiscard]] Result EmitCall(FunctionId id, std::uint8_t argc) noexcept;
    // AndJump, OrJump, JumpIfFalse, Jump
    [[nodiscard]] std::expected<JumpRef, CompileErrorCode> EmitJump(Op op) noexcept;
    [[nodiscard]] Result PatchJump(JumpRef ref) noexcept; // the target is the next instruction
    [[nodiscard]] Result EmitEnd() noexcept;

    [[nodiscard]] std::span<const Instruction> Code() const noexcept {
        return std::span<const Instruction>{code_.data(), size_};
    }

    [[nodiscard]] std::span<const NameRef> Names() const noexcept {
        return std::span<const NameRef>{names_.data(), name_count_};
    }

    [[nodiscard]] std::uint8_t MaxDepth() const noexcept {
        return max_materialized_;
    }

private:
    struct Operand {
        bool pending;
        Value value; // pending only
    };

    // Beyond max_stack materialized values, the operand stack also holds the pending constants of
    // the arguments being collected and of the enclosing levels.
    static constexpr std::size_t kOperandCapacity = limits::kMaxStack + limits::kMaxArguments + limits::kMaxNesting + 2;

    [[nodiscard]] Result Append(Instruction instruction, int stack_delta) noexcept;
    [[nodiscard]] Result PushOperand(Operand operand) noexcept;
    [[nodiscard]] Result Flush() noexcept;
    [[nodiscard]] std::size_t PendingCount() const noexcept;
    [[nodiscard]] std::expected<std::uint16_t, CompileErrorCode> Intern(std::string_view name) noexcept;

    std::string_view text_;
    Limits limits_;

    std::array<Instruction, limits::kMaxInstructions> code_{};
    std::uint16_t size_ = 0;

    std::array<NameRef, limits::kMaxVariables> names_{};
    std::uint16_t name_count_ = 0;

    std::array<Operand, kOperandCapacity> operands_{};
    std::size_t operand_count_ = 0;
    std::uint8_t materialized_ = 0; // values on the evaluation stack at this point of the code
    std::uint8_t max_materialized_ = 0;
};

} // namespace eerie_leap::expression_engine::detail
