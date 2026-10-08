#pragma once

// Shared behaviour table: expression, variable bindings, expected value. Consumed by the GoogleTest
// suite and by tests/zephyr. Expected values are what the same float operations produce, so they
// are compared bit for bit (see support/approx.hpp); `ulps` relaxes the few that go through libm
// paths that may differ between platforms.

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numbers>
#include <string_view>

#include <eerie_leap/expression_engine/types.hpp>

namespace eerie_leap::expression_engine::testing {

struct Binding {
    std::string_view name;
    Value value;
};

struct EvaluationVector {
    std::string_view expression;
    std::array<Binding, 3> bindings; // unused entries have an empty name
    Value expected;
    std::uint32_t ulps = 0;
};

inline constexpr Value kNaN = std::numeric_limits<Value>::quiet_NaN();
inline constexpr Value kInf = std::numeric_limits<Value>::infinity();

inline constexpr std::array<EvaluationVector, 100> kEvaluationVectors{{
    // arithmetic, precedence, associativity
    {"1 + 2 * 3", {}, 7.0f},
    {"(1 + 2) * 3", {}, 9.0f},
    {"-2^2", {}, -4.0f},
    {"(-2)^2", {}, 4.0f},
    {"2^3^2", {}, 512.0f},
    {"2^-1", {}, 0.5f},
    {"10 / 4", {}, 2.5f},
    {"1 - 2 - 3", {}, -4.0f},
    {"2 * 3 / 4", {}, 1.5f},
    {"+5", {}, 5.0f},
    {"--5", {}, 5.0f},
    {"- -5", {}, 5.0f},
    {"2 * -3", {}, -6.0f},
    {"1.5e1 + .5", {}, 15.5f},
    {"1E-1 * 10", {}, 1.0f},
    {"x", {{{"x", 8.0f}}}, 8.0f},
    {"x * 4 + 1.6", {{{"x", 2.0f}}}, 9.6f},
    {"(x + y) * 4", {{{"x", 8.0f}, {"y", 2.0f}}}, 40.0f},
    {"x * 2 + sensor_2 + 1", {{{"x", 3.0f}, {"sensor_2", 0.5f}}}, 7.5f},
    {"sensor_1 + 8.34", {{{"sensor_1", 1.66f}}}, 10.0f},
    {"sensor_1 < 400", {{{"sensor_1", 399.0f}}}, 1.0f},
    {"(x - 8 * var_d) / f", {{{"x", 20.0f}, {"var_d", 2.0f}, {"f", 2.0f}}}, 2.0f},
    {"x + x + x", {{{"x", 1.5f}}}, 4.5f},
    {"1 + 2 * x", {{{"x", 4.0f}}}, 9.0f},
    // comparisons (non-chaining)
    {"1 < 2 < 3", {}, 1.0f},
    {"3 > 2 > 1", {}, 0.0f},
    {"1 == 1", {}, 1.0f},
    {"1 != 1", {}, 0.0f},
    {"2 <= 2", {}, 1.0f},
    {"2 >= 3", {}, 0.0f},
    {"x == 2.5", {{{"x", 2.5f}}}, 1.0f},
    // logic
    {"1 && 0", {}, 0.0f},
    {"1 && 2", {}, 1.0f},
    {"0 || 2", {}, 1.0f},
    {"0 || 0", {}, 0.0f},
    {"1 || 0 && 0", {}, 1.0f},
    {"(1 || 0) && 0", {}, 0.0f},
    {"x && y", {{{"x", 3.0f}, {"y", -1.0f}}}, 1.0f},
    {"x || y", {{{"x", 0.0f}, {"y", 0.0f}}}, 0.0f},
    {"0 && (1 / 0)", {}, 0.0f},
    {"1 || (0 / 0)", {}, 1.0f},
    // ternary
    {"1 ? 2 : 3", {}, 2.0f},
    {"0 ? 2 : 3", {}, 3.0f},
    {"1 ? 0 ? 5 : 6 : 7", {}, 6.0f},
    {"0 ? 1 : 1 ? 2 : 3", {}, 2.0f},
    {"x > 1 ? x : -x", {{{"x", -3.0f}}}, 3.0f},
    {"(1 ? 2 : 3) + 4", {}, 6.0f},
    {"(0 ? 2 : 3) + 4", {}, 7.0f},
    {"x ? y : z", {{{"x", 0.0f}, {"y", 1.0f}, {"z", 2.0f}}}, 2.0f},
    // constants
    {"_pi", {}, std::numbers::pi_v<Value>},
    {"2 * _pi", {}, 2.0f * std::numbers::pi_v<Value>},
    {"_e", {}, std::numbers::e_v<Value>},
    // IEEE behaviour
    {"1 / 0", {}, kInf},
    {"-1 / 0", {}, -kInf},
    {"0 / 0", {}, kNaN},
    {"(0 / 0) == (0 / 0)", {}, 0.0f},
    {"(0 / 0) < 1", {}, 0.0f},
    {"(0 / 0) != 1", {}, 1.0f},
    {"(0 / 0) ? 1 : 2", {}, 1.0f},
    {"(0 / 0) && 1", {}, 1.0f},
    {"0 && (0 / 0)", {}, 0.0f},
    {"(0 / 0) || 0", {}, 1.0f},
    {"x && 1", {{{"x", kNaN}}}, 1.0f},
    {"x + 1", {{{"x", kNaN}}}, kNaN},
    {"x ? 1 : 2", {{{"x", kNaN}}}, 1.0f},
    // bitwise operators and integer literals
    {"6 & 3", {}, 2.0f},
    {"6 | 3", {}, 7.0f},
    {"xor(6, 3)", {}, 5.0f},
    {"~0", {}, -1.0f},
    {"~5", {}, -6.0f},
    {"1 << 4", {}, 16.0f},
    {"256 >> 4", {}, 16.0f},
    {"-8 >> 1", {}, -4.0f},
    {"1 << 31", {}, -2147483648.0f},
    {"1 << 32", {}, 0.0f},
    {"1 << -1", {}, 0.0f},
    {"8 >> 32", {}, 0.0f},
    {"3.7 & 1", {}, 1.0f},
    {"-3.7 & 1", {}, 1.0f},
    {"4294967808 & 65535", {}, 512.0f},
    {"-1 & 255", {}, 255.0f},
    {"0xFF", {}, 255.0f},
    {"0xFFFFFFFF", {}, -1.0f},
    {"0xFFFF0000", {}, -65536.0f},
    {"0x80000000", {}, -2147483648.0f},
    {"0b1010", {}, 10.0f},
    {"0b1010 & 0B0110", {}, 2.0f},
    {"x & 0xFF00 >> 8", {{{"x", 4660.0f}}}, 52.0f},
    {"(x >> 8) & 0xFF", {{{"x", 4660.0f}}}, 18.0f},
    {"x & 4 == 4", {{{"x", 6.0f}}}, 1.0f},
    {"1 | 2 & 3", {}, 3.0f},
    {"1 << 2 + 1", {}, 8.0f},
    {"~0 & 255", {}, 255.0f},
    {"-x >> 1", {{{"x", 8.0f}}}, -4.0f},
    {"2 * 3 & 2", {}, 2.0f},
    {"1 + 2 << 1", {}, 6.0f},
    {"(0 / 0) & 1", {}, 0.0f},
    {"(1 / 0) | 0", {}, 0.0f},
    {"1e10 & 0xFFFF", {}, 58368.0f},
    {"1e30 & 1", {}, 0.0f},
}};

} // namespace eerie_leap::expression_engine::testing
