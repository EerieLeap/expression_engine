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

inline constexpr Value NAN_VALUE = std::numeric_limits<Value>::quiet_NaN();
inline constexpr Value INF_VALUE = std::numeric_limits<Value>::infinity();

inline constexpr std::array<EvaluationVector, 100> EVALUATION_VECTORS{ {
    // arithmetic, precedence, associativity
    { "1 + 2 * 3", {}, 7.0F },
    { "(1 + 2) * 3", {}, 9.0F },
    { "-2^2", {}, -4.0F },
    { "(-2)^2", {}, 4.0F },
    { "2^3^2", {}, 512.0F },
    { "2^-1", {}, 0.5F },
    { "10 / 4", {}, 2.5F },
    { "1 - 2 - 3", {}, -4.0F },
    { "2 * 3 / 4", {}, 1.5F },
    { "+5", {}, 5.0F },
    { "--5", {}, 5.0F },
    { "- -5", {}, 5.0F },
    { "2 * -3", {}, -6.0F },
    { "1.5e1 + .5", {}, 15.5F },
    { "1E-1 * 10", {}, 1.0F },
    { "x", { { { "x", 8.0F } } }, 8.0F },
    { "x * 4 + 1.6", { { { "x", 2.0F } } }, 9.6F },
    { "(x + y) * 4", { { { "x", 8.0F }, { "y", 2.0F } } }, 40.0F },
    { "x * 2 + sensor_2 + 1", { { { "x", 3.0F }, { "sensor_2", 0.5F } } }, 7.5F },
    { "sensor_1 + 8.34", { { { "sensor_1", 1.66F } } }, 10.0F },
    { "sensor_1 < 400", { { { "sensor_1", 399.0F } } }, 1.0F },
    { "(x - 8 * var_d) / f", { { { "x", 20.0F }, { "var_d", 2.0F }, { "f", 2.0F } } }, 2.0F },
    { "x + x + x", { { { "x", 1.5F } } }, 4.5F },
    { "1 + 2 * x", { { { "x", 4.0F } } }, 9.0F },
    // comparisons (non-chaining)
    { "1 < 2 < 3", {}, 1.0F },
    { "3 > 2 > 1", {}, 0.0F },
    { "1 == 1", {}, 1.0F },
    { "1 != 1", {}, 0.0F },
    { "2 <= 2", {}, 1.0F },
    { "2 >= 3", {}, 0.0F },
    { "x == 2.5", { { { "x", 2.5F } } }, 1.0F },
    // logic
    { "1 && 0", {}, 0.0F },
    { "1 && 2", {}, 1.0F },
    { "0 || 2", {}, 1.0F },
    { "0 || 0", {}, 0.0F },
    { "1 || 0 && 0", {}, 1.0F },
    { "(1 || 0) && 0", {}, 0.0F },
    { "x && y", { { { "x", 3.0F }, { "y", -1.0F } } }, 1.0F },
    { "x || y", { { { "x", 0.0F }, { "y", 0.0F } } }, 0.0F },
    { "0 && (1 / 0)", {}, 0.0F },
    { "1 || (0 / 0)", {}, 1.0F },
    // ternary
    { "1 ? 2 : 3", {}, 2.0F },
    { "0 ? 2 : 3", {}, 3.0F },
    { "1 ? 0 ? 5 : 6 : 7", {}, 6.0F },
    { "0 ? 1 : 1 ? 2 : 3", {}, 2.0F },
    { "x > 1 ? x : -x", { { { "x", -3.0F } } }, 3.0F },
    { "(1 ? 2 : 3) + 4", {}, 6.0F },
    { "(0 ? 2 : 3) + 4", {}, 7.0F },
    { "x ? y : z", { { { "x", 0.0F }, { "y", 1.0F }, { "z", 2.0F } } }, 2.0F },
    // constants
    { "_pi", {}, std::numbers::pi_v<Value> },
    { "2 * _pi", {}, 2.0F * std::numbers::pi_v<Value> },
    { "_e", {}, std::numbers::e_v<Value> },
    // IEEE behaviour
    { "1 / 0", {}, INF_VALUE },
    { "-1 / 0", {}, -INF_VALUE },
    { "0 / 0", {}, NAN_VALUE },
    { "(0 / 0) == (0 / 0)", {}, 0.0F },
    { "(0 / 0) < 1", {}, 0.0F },
    { "(0 / 0) != 1", {}, 1.0F },
    { "(0 / 0) ? 1 : 2", {}, 1.0F },
    { "(0 / 0) && 1", {}, 1.0F },
    { "0 && (0 / 0)", {}, 0.0F },
    { "(0 / 0) || 0", {}, 1.0F },
    { "x && 1", { { { "x", NAN_VALUE } } }, 1.0F },
    { "x + 1", { { { "x", NAN_VALUE } } }, NAN_VALUE },
    { "x ? 1 : 2", { { { "x", NAN_VALUE } } }, 1.0F },
    // bitwise operators and integer literals
    { "6 & 3", {}, 2.0F },
    { "6 | 3", {}, 7.0F },
    { "xor(6, 3)", {}, 5.0F },
    { "~0", {}, -1.0F },
    { "~5", {}, -6.0F },
    { "1 << 4", {}, 16.0F },
    { "256 >> 4", {}, 16.0F },
    { "-8 >> 1", {}, -4.0F },
    { "1 << 31", {}, -2147483648.0F },
    { "1 << 32", {}, 0.0F },
    { "1 << -1", {}, 0.0F },
    { "8 >> 32", {}, 0.0F },
    { "3.7 & 1", {}, 1.0F },
    { "-3.7 & 1", {}, 1.0F },
    { "4294967808 & 65535", {}, 512.0F },
    { "-1 & 255", {}, 255.0F },
    { "0xFF", {}, 255.0F },
    { "0xFFFFFFFF", {}, -1.0F },
    { "0xFFFF0000", {}, -65536.0F },
    { "0x80000000", {}, -2147483648.0F },
    { "0b1010", {}, 10.0F },
    { "0b1010 & 0B0110", {}, 2.0F },
    { "x & 0xFF00 >> 8", { { { "x", 4660.0F } } }, 52.0F },
    { "(x >> 8) & 0xFF", { { { "x", 4660.0F } } }, 18.0F },
    { "x & 4 == 4", { { { "x", 6.0F } } }, 1.0F },
    { "1 | 2 & 3", {}, 3.0F },
    { "1 << 2 + 1", {}, 8.0F },
    { "~0 & 255", {}, 255.0F },
    { "-x >> 1", { { { "x", 8.0F } } }, -4.0F },
    { "2 * 3 & 2", {}, 2.0F },
    { "1 + 2 << 1", {}, 6.0F },
    { "(0 / 0) & 1", {}, 0.0F },
    { "(1 / 0) | 0", {}, 0.0F },
    { "1e10 & 0xFFFF", {}, 58368.0F },
    { "1e30 & 1", {}, 0.0F },
} };

} // namespace eerie_leap::expression_engine::testing
