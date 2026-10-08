#include "eerie_leap/expression_engine/program.hpp"

#include <array>
#include <limits>

#include "eerie_leap/expression_engine/functions.hpp"

#include "ops.hpp"

namespace eerie_leap::expression_engine {

Value Program::Evaluate(std::span<const Value> variables) const noexcept {
    if(variables.size() < names_.size()) {
        return std::numeric_limits<Value>::quiet_NaN();
    }

    // Sized by the cap, not by max_stack_, so the frame is the same for every program.
    std::array<Value, limits::kMaxStack> stack{};
    std::size_t sp = 0;
    const Instruction* pc = code_.data();

    for(;;) {
        const Instruction in = *pc++;

        switch(in.op) {
        case Op::PushConst:
            stack[sp++] = in.imm;
            break;

        case Op::PushVar:
            stack[sp++] = variables[in.arg];
            break;

        case Op::Neg:
        case Op::BitNot:
        case Op::ToBool:
            stack[sp - 1] = detail::ApplyUnary(in.op, stack[sp - 1]);
            break;

        case Op::Add:
        case Op::Sub:
        case Op::Mul:
        case Op::Div:
        case Op::Pow:
        case Op::Lt:
        case Op::Gt:
        case Op::Le:
        case Op::Ge:
        case Op::Eq:
        case Op::Ne:
        case Op::BitAnd:
        case Op::BitOr:
        case Op::Shl:
        case Op::Shr:
            --sp;
            stack[sp - 1] = detail::ApplyBinary(in.op, stack[sp - 1], stack[sp]);
            break;

        case Op::AndJump:
            if(stack[--sp] == 0.0f) {
                stack[sp++] = 0.0f;
                pc = code_.data() + in.arg;
            }
            break;

        case Op::OrJump:
            if(stack[--sp] != 0.0f) {
                stack[sp++] = 1.0f;
                pc = code_.data() + in.arg;
            }
            break;

        case Op::JumpIfFalse:
            if(stack[--sp] == 0.0f) {
                pc = code_.data() + in.arg;
            }
            break;

        case Op::Jump:
            pc = code_.data() + in.arg;
            break;

        case Op::Call: {
            const std::size_t argc = in.arg;
            sp -= argc;
            stack[sp] = Invoke(static_cast<FunctionId>(in.aux), std::span<const Value>{stack.data() + sp, argc});
            ++sp;
            break;
        }

        case Op::End:
            return stack[0];
        }
    }
}

std::optional<std::size_t> Program::VariableIndex(std::string_view name) const noexcept {
    for(std::size_t index = 0; index < names_.size(); ++index) {
        if(VariableName(index) == name) {
            return index;
        }
    }
    return std::nullopt;
}

} // namespace eerie_leap::expression_engine
