#include "emitter.hpp"

#include <algorithm>

#include "ops.hpp"

namespace eerie_leap::expression_engine::detail {

Emitter::Emitter(std::string_view text, const Limits& limits) noexcept : text_(text), limits_(limits) {}

Emitter::Result Emitter::Append(Instruction instruction, int stack_delta) noexcept {
    if(size_ >= limits_.max_instructions) {
        return std::unexpected(CompileErrorCode::TooManyInstructions);
    }
    code_[size_++] = instruction;

    const int depth = static_cast<int>(materialized_) + stack_delta;
    if(depth > static_cast<int>(limits_.max_stack)) {
        return std::unexpected(CompileErrorCode::StackTooDeep);
    }
    materialized_ = static_cast<std::uint8_t>(depth);
    max_materialized_ = std::max(max_materialized_, materialized_);
    return {};
}

Emitter::Result Emitter::PushOperand(Operand operand) noexcept {
    if(operand_count_ >= kOperandCapacity) {
        return std::unexpected(CompileErrorCode::StackTooDeep);
    }
    operands_[operand_count_++] = operand;
    return {};
}

std::size_t Emitter::PendingCount() const noexcept {
    std::size_t count = 0;
    while(count < operand_count_ && operands_[operand_count_ - 1 - count].pending) {
        ++count;
    }
    return count;
}

Emitter::Result Emitter::Flush() noexcept {
    const std::size_t first = operand_count_ - PendingCount();
    for(std::size_t i = first; i < operand_count_; ++i) {
        if(const auto r = Append(Instruction{Op::PushConst, 0, 0, operands_[i].value}, +1); !r) {
            return r;
        }
        operands_[i].pending = false;
    }
    return {};
}

std::expected<std::uint16_t, CompileErrorCode> Emitter::Intern(std::string_view name) noexcept {
    for(std::uint16_t index = 0; index < name_count_; ++index) {
        if(text_.substr(names_[index].offset, names_[index].length) == name) {
            return index;
        }
    }
    if(name_count_ >= limits_.max_variables) {
        return std::unexpected(CompileErrorCode::TooManyVariables);
    }
    names_[name_count_] =
        NameRef{static_cast<std::uint16_t>(name.data() - text_.data()), static_cast<std::uint16_t>(name.size())};
    return name_count_++;
}

Emitter::Result Emitter::EmitConst(Value value) noexcept {
    return PushOperand(Operand{true, value});
}

Emitter::Result Emitter::EmitVariable(std::string_view name) noexcept {
    if(const auto r = Flush(); !r) {
        return r;
    }
    const auto index = Intern(name);
    if(!index) {
        return std::unexpected(index.error());
    }
    if(const auto r = Append(Instruction{Op::PushVar, 0, *index, 0.0f}, +1); !r) {
        return r;
    }
    return PushOperand(Operand{false, 0.0f});
}

Emitter::Result Emitter::EmitUnary(Op op) noexcept {
    Operand& top = operands_[operand_count_ - 1];
    if(top.pending) {
        top.value = ApplyUnary(op, top.value);
        return {};
    }
    return Append(Instruction{op, 0, 0, 0.0f}, 0);
}

Emitter::Result Emitter::EmitBinary(Op op) noexcept {
    Operand& lhs = operands_[operand_count_ - 2];
    const Operand& rhs = operands_[operand_count_ - 1];

    if(lhs.pending && rhs.pending) {
        lhs.value = ApplyBinary(op, lhs.value, rhs.value);
        --operand_count_;
        return {};
    }

    if(const auto r = Flush(); !r) {
        return r;
    }
    if(const auto r = Append(Instruction{op, 0, 0, 0.0f}, -1); !r) {
        return r;
    }
    --operand_count_;
    operands_[operand_count_ - 1] = Operand{false, 0.0f};
    return {};
}

Emitter::Result Emitter::EmitCall(FunctionId id, std::uint8_t argc) noexcept {
    const std::size_t first = operand_count_ - argc;

    if(PendingCount() >= argc) {
        std::array<Value, limits::kMaxArguments> args{};
        for(std::size_t i = 0; i < argc; ++i) {
            args[i] = operands_[first + i].value;
        }
        operands_[first] = Operand{true, Invoke(id, std::span<const Value>{args.data(), argc})};
        operand_count_ = first + 1;
        return {};
    }

    if(const auto r = Flush(); !r) {
        return r;
    }
    if(const auto r = Append(Instruction{Op::Call, std::to_underlying(id), argc, 0.0f}, 1 - static_cast<int>(argc));
       !r) {
        return r;
    }
    operand_count_ = first;
    return PushOperand(Operand{false, 0.0f});
}

std::expected<JumpRef, CompileErrorCode> Emitter::EmitJump(Op op) noexcept {
    if(const auto r = Flush(); !r) {
        return std::unexpected(r.error());
    }

    const auto index = size_;
    const bool pops = op != Op::Jump;
    if(pops) {
        --operand_count_;
    }
    if(const auto r = Append(Instruction{op, 0, 0, 0.0f}, pops ? -1 : 0); !r) {
        return std::unexpected(r.error());
    }

    // AndJump and OrJump push the result on the taken path; JumpIfFalse and Jump push nothing.
    const bool pushes = op == Op::AndJump || op == Op::OrJump;
    const auto extra = static_cast<std::uint8_t>(pushes ? 1 : 0);
    return JumpRef{
        index,
        static_cast<std::uint8_t>(materialized_ + extra),
        static_cast<std::uint8_t>(operand_count_ + extra)
    };
}

Emitter::Result Emitter::PatchJump(JumpRef ref) noexcept {
    if(const auto r = Flush(); !r) {
        return r;
    }
    code_[ref.index].arg = size_;
    materialized_ = ref.arrival_depth;
    operand_count_ = ref.arrival_operands;
    return {};
}

Emitter::Result Emitter::EmitEnd() noexcept {
    if(const auto r = Flush(); !r) {
        return r;
    }
    return Append(Instruction{Op::End, 0, 0, 0.0f}, 0);
}

} // namespace eerie_leap::expression_engine::detail
