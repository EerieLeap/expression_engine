#pragma once

// Every function with a constant argument (folded at compile time) and the same call with the
// argument in a variable (evaluated at run time). Expected values use the documented formulas,
// computed at static initialization with <cmath>, so this table is `inline const`, not constexpr.
// The arguments go through Runtime() so the compiler cannot fold the <cmath> call itself with a
// correctly rounded result: the expected value must come from the same libm the engine calls,
// which on a target such as picolibc can differ from the host's by an ulp.

#include <array>
#include <cmath>
#include <limits>
#include <numbers>
#include <string_view>

#include <eerie_leap/expression_engine/types.hpp>

#include "vectors/evaluation_vectors.hpp"

namespace eerie_leap::expression_engine::testing {

// Defeats compile-time evaluation of the <cmath> call that receives the result.
inline Value Runtime(Value value) {
    volatile Value escaped = value;
    return escaped;
}

struct FunctionVector {
    std::string_view name;
    std::array<Value, 3> args; // NaN marks an unused slot
    Value expected;
    std::uint32_t ulps = 0;
};

inline const std::array<FunctionVector, 40> kFunctionVectors{{
    {"abs", {-3.0f, kNaN, kNaN}, 3.0f},
    {"acos", {0.5f, kNaN, kNaN}, std::acos(Runtime(0.5f))},
    {"acosh", {2.0f, kNaN, kNaN}, std::acosh(Runtime(2.0f)), 2},
    {"asin", {0.5f, kNaN, kNaN}, std::asin(Runtime(0.5f))},
    {"asinh", {1.0f, kNaN, kNaN}, std::asinh(Runtime(1.0f)), 2},
    {"atan", {1.0f, kNaN, kNaN}, std::atan(Runtime(1.0f))},
    {"atan2", {1.0f, 2.0f, kNaN}, std::atan2(Runtime(1.0f), Runtime(2.0f))},
    {"atanh", {0.5f, kNaN, kNaN}, std::atanh(Runtime(0.5f)), 2},
    {"avg", {1.0f, 2.0f, 4.0f}, 7.0f / 3.0f},
    {"cos", {1.0f, kNaN, kNaN}, std::cos(Runtime(1.0f))},
    {"cosh", {1.0f, kNaN, kNaN}, std::cosh(Runtime(1.0f)), 2},
    {"exp", {2.0f, kNaN, kNaN}, std::exp(Runtime(2.0f))},
    {"ln", {10.0f, kNaN, kNaN}, std::log(Runtime(10.0f))},
    {"log", {10.0f, kNaN, kNaN}, std::log(Runtime(10.0f))},
    {"log10", {1000.0f, kNaN, kNaN}, std::log10(Runtime(1000.0f))},
    {"log2", {8.0f, kNaN, kNaN}, std::log(Runtime(8.0f)) / std::log(Runtime(2.0f)), 2},
    {"max", {3.0f, 1.0f, 2.0f}, 3.0f},
    {"min", {3.0f, 1.0f, 2.0f}, 1.0f},
    {"rint", {2.5f, kNaN, kNaN}, 3.0f},
    {"rint", {-2.5f, kNaN, kNaN}, -2.0f},
    {"rint", {2.4f, kNaN, kNaN}, 2.0f},
    {"rint", {-0.4f, kNaN, kNaN}, 0.0f},
    {"sign", {-3.0f, kNaN, kNaN}, -1.0f},
    {"sign", {0.0f, kNaN, kNaN}, 0.0f},
    {"sign", {2.5f, kNaN, kNaN}, 1.0f},
    {"sign", {kNaN, kNaN, kNaN}, 0.0f},
    {"sin", {1.0f, kNaN, kNaN}, std::sin(Runtime(1.0f))},
    {"sinh", {1.0f, kNaN, kNaN}, std::sinh(Runtime(1.0f)), 2},
    {"sqrt", {2.0f, kNaN, kNaN}, std::sqrt(Runtime(2.0f))},
    {"sqrt", {-1.0f, kNaN, kNaN}, kNaN},
    {"sum", {1.0f, 2.0f, 3.5f}, 6.5f},
    {"sum", {5.0f, kNaN, kNaN}, 5.0f},
    {"tan", {1.0f, kNaN, kNaN}, std::tan(Runtime(1.0f))},
    {"tanh", {1.0f, kNaN, kNaN}, std::tanh(Runtime(1.0f)), 2},
    {"xor", {6.0f, 3.0f, kNaN}, 5.0f},
    {"xor", {-1.0f, 255.0f, kNaN}, -256.0f},
    {"log", {0.0f, kNaN, kNaN}, -kInf},
    {"min", {5.0f, kNaN, kNaN}, 5.0f},
    {"max", {-1.0f, -2.0f, kNaN}, -1.0f},
    {"avg", {1.0f, 2.0f, kNaN}, 1.5f},
}};

} // namespace eerie_leap::expression_engine::testing
