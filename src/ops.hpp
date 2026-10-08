#pragma once

#include <cmath>
#include <limits>

#include "eerie_leap/expression_engine/instruction.hpp"
#include "eerie_leap/expression_engine/types.hpp"

#include "bit_ops.hpp"

// The semantics of every arithmetic, comparison and bitwise opcode, in one place. The evaluator
// and the constant folder both call these, so compile-time and run-time results are identical.

namespace eerie_leap::expression_engine::detail {

[[nodiscard]] constexpr bool IsBinary(Op op) noexcept {
    return op >= Op::Add && op <= Op::Shr && op != Op::BitNot;
}

[[nodiscard]] constexpr bool IsUnary(Op op) noexcept {
    return op == Op::Neg || op == Op::BitNot || op == Op::ToBool;
}

[[nodiscard]] inline Value ApplyBinary(Op op, Value a, Value b) noexcept {
    switch(op) {
    case Op::Add:
        return a + b;
    case Op::Sub:
        return a - b;
    case Op::Mul:
        return a * b;
    case Op::Div:
        return a / b;
    case Op::Pow:
        return std::pow(a, b);
    case Op::Lt:
        return a < b ? 1.0f : 0.0f;
    case Op::Gt:
        return a > b ? 1.0f : 0.0f;
    case Op::Le:
        return a <= b ? 1.0f : 0.0f;
    case Op::Ge:
        return a >= b ? 1.0f : 0.0f;
    case Op::Eq:
        return a == b ? 1.0f : 0.0f;
    case Op::Ne:
        return a != b ? 1.0f : 0.0f;
    case Op::BitAnd:
        return BitAnd(a, b);
    case Op::BitOr:
        return BitOr(a, b);
    case Op::Shl:
        return Shl(a, b);
    case Op::Shr:
        return Shr(a, b);
    default:
        return std::numeric_limits<Value>::quiet_NaN();
    }
}

[[nodiscard]] inline Value ApplyUnary(Op op, Value a) noexcept {
    switch(op) {
    case Op::Neg:
        return -a;
    case Op::BitNot:
        return BitNot(a);
    case Op::ToBool:
        return a != 0.0f ? 1.0f : 0.0f;
    default:
        return std::numeric_limits<Value>::quiet_NaN();
    }
}

} // namespace eerie_leap::expression_engine::detail
