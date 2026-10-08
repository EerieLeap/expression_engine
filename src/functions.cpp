#include "eerie_leap/expression_engine/functions.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

#include "bit_ops.hpp"

namespace eerie_leap::expression_engine {

// The formulas are the documented ones (docs/LANGUAGE.md) and are part of the language, since a
// stored expression must keep its value: rint is floor(v + 0.5), log2 is log(v) / log(2), sum and
// avg accumulate in argument order, min and max fold with std::min / std::max from the first
// argument.
Value Invoke(FunctionId id, std::span<const Value> args) noexcept {
    switch(id) {
    case FunctionId::Abs:
        return std::fabs(args[0]);
    case FunctionId::Acos:
        return std::acos(args[0]);
    case FunctionId::Acosh:
        return std::acosh(args[0]);
    case FunctionId::Asin:
        return std::asin(args[0]);
    case FunctionId::Asinh:
        return std::asinh(args[0]);
    case FunctionId::Atan:
        return std::atan(args[0]);
    case FunctionId::Atan2:
        return std::atan2(args[0], args[1]);
    case FunctionId::Atanh:
        return std::atanh(args[0]);
    case FunctionId::Avg: {
        Value sum = 0.0f;
        for(const Value arg : args) {
            sum += arg;
        }
        return sum / static_cast<Value>(args.size());
    }
    case FunctionId::Cos:
        return std::cos(args[0]);
    case FunctionId::Cosh:
        return std::cosh(args[0]);
    case FunctionId::Exp:
        return std::exp(args[0]);
    case FunctionId::Ln:
    case FunctionId::Log:
        return std::log(args[0]);
    case FunctionId::Log10:
        return std::log10(args[0]);
    case FunctionId::Log2:
        return std::log(args[0]) / std::log(2.0f);
    case FunctionId::Max: {
        Value result = args[0];
        for(const Value arg : args) {
            result = std::max(result, arg);
        }
        return result;
    }
    case FunctionId::Min: {
        Value result = args[0];
        for(const Value arg : args) {
            result = std::min(result, arg);
        }
        return result;
    }
    case FunctionId::Rint:
        return std::floor(args[0] + 0.5f);
    case FunctionId::Sign:
        return args[0] < 0.0f ? -1.0f : (args[0] > 0.0f ? 1.0f : 0.0f);
    case FunctionId::Sin:
        return std::sin(args[0]);
    case FunctionId::Sinh:
        return std::sinh(args[0]);
    case FunctionId::Sqrt:
        return std::sqrt(args[0]);
    case FunctionId::Sum: {
        Value sum = 0.0f;
        for(const Value arg : args) {
            sum += arg;
        }
        return sum;
    }
    case FunctionId::Tan:
        return std::tan(args[0]);
    case FunctionId::Tanh:
        return std::tanh(args[0]);
    case FunctionId::Xor:
        return detail::BitXor(args[0], args[1]);
    }
    return std::numeric_limits<Value>::quiet_NaN();
}

} // namespace eerie_leap::expression_engine
