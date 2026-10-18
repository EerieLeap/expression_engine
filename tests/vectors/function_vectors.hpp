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
    const volatile Value escaped = value;
    return escaped;
}

struct FunctionVector {
    std::string_view name;
    std::array<Value, 3> args; // NaN marks an unused slot
    Value expected;
    std::uint32_t ulps = 0;
};

inline const std::array<FunctionVector, 40> kFunctionVectors{ {
    { "abs", { -3.0F, kNaN, kNaN }, 3.0F },
    { "acos", { 0.5F, kNaN, kNaN }, std::acos(Runtime(0.5F)) },
    { "acosh", { 2.0F, kNaN, kNaN }, std::acosh(Runtime(2.0F)), 2 },
    { "asin", { 0.5F, kNaN, kNaN }, std::asin(Runtime(0.5F)) },
    { "asinh", { 1.0F, kNaN, kNaN }, std::asinh(Runtime(1.0F)), 2 },
    { "atan", { 1.0F, kNaN, kNaN }, std::atan(Runtime(1.0F)) },
    { "atan2", { 1.0F, 2.0F, kNaN }, std::atan2(Runtime(1.0F), Runtime(2.0F)) },
    { "atanh", { 0.5F, kNaN, kNaN }, std::atanh(Runtime(0.5F)), 2 },
    { "avg", { 1.0F, 2.0F, 4.0F }, 7.0F / 3.0F },
    { "cos", { 1.0F, kNaN, kNaN }, std::cos(Runtime(1.0F)) },
    { "cosh", { 1.0F, kNaN, kNaN }, std::cosh(Runtime(1.0F)), 2 },
    { "exp", { 2.0F, kNaN, kNaN }, std::exp(Runtime(2.0F)) },
    { "ln", { 10.0F, kNaN, kNaN }, std::log(Runtime(10.0F)) },
    { "log", { 10.0F, kNaN, kNaN }, std::log(Runtime(10.0F)) },
    { "log10", { 1000.0F, kNaN, kNaN }, std::log10(Runtime(1000.0F)) },
    { "log2", { 8.0F, kNaN, kNaN }, std::log(Runtime(8.0F)) / std::log(Runtime(2.0F)), 2 },
    { "max", { 3.0F, 1.0F, 2.0F }, 3.0F },
    { "min", { 3.0F, 1.0F, 2.0F }, 1.0F },
    { "rint", { 2.5F, kNaN, kNaN }, 3.0F },
    { "rint", { -2.5F, kNaN, kNaN }, -2.0F },
    { "rint", { 2.4F, kNaN, kNaN }, 2.0F },
    { "rint", { -0.4F, kNaN, kNaN }, 0.0F },
    { "sign", { -3.0F, kNaN, kNaN }, -1.0F },
    { "sign", { 0.0F, kNaN, kNaN }, 0.0F },
    { "sign", { 2.5F, kNaN, kNaN }, 1.0F },
    { "sign", { kNaN, kNaN, kNaN }, 0.0F },
    { "sin", { 1.0F, kNaN, kNaN }, std::sin(Runtime(1.0F)) },
    { "sinh", { 1.0F, kNaN, kNaN }, std::sinh(Runtime(1.0F)), 2 },
    { "sqrt", { 2.0F, kNaN, kNaN }, std::sqrt(Runtime(2.0F)) },
    { "sqrt", { -1.0F, kNaN, kNaN }, kNaN },
    { "sum", { 1.0F, 2.0F, 3.5F }, 6.5F },
    { "sum", { 5.0F, kNaN, kNaN }, 5.0F },
    { "tan", { 1.0F, kNaN, kNaN }, std::tan(Runtime(1.0F)) },
    { "tanh", { 1.0F, kNaN, kNaN }, std::tanh(Runtime(1.0F)), 2 },
    { "xor", { 6.0F, 3.0F, kNaN }, 5.0F },
    { "xor", { -1.0F, 255.0F, kNaN }, -256.0F },
    { "log", { 0.0F, kNaN, kNaN }, -kInf },
    { "min", { 5.0F, kNaN, kNaN }, 5.0F },
    { "max", { -1.0F, -2.0F, kNaN }, -1.0F },
    { "avg", { 1.0F, 2.0F, kNaN }, 1.5F },
} };

} // namespace eerie_leap::expression_engine::testing
