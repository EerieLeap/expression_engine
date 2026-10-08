#pragma once

#include <bit>
#include <cmath>
#include <cstdint>

#include "eerie_leap/expression_engine/types.hpp"

// The 32-bit integer view of a Value used by the bitwise operators and the integer literals.
// This is the only code that knows the conversion rules (docs/LANGUAGE.md, "Bitwise operations").

namespace eerie_leap::expression_engine::detail {

// JavaScript's ToInt32: NaN and infinities give 0, the fraction is dropped, out-of-range values
// wrap modulo 2^32 and the result is read as two's complement.
[[nodiscard]] inline std::int32_t ToInt32(Value v) noexcept {
    // A float of magnitude 2^63 or more has zeros in its low 32 bits, so 0 is also the wrapped value.
    if(!std::isfinite(v) || std::fabs(v) >= 9223372036854775808.0f) {
        return 0;
    }
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(v)));
}

[[nodiscard]] inline Value FromInt32(std::int32_t i) noexcept {
    return static_cast<Value>(i);
}

[[nodiscard]] inline Value BitAnd(Value a, Value b) noexcept {
    return FromInt32(ToInt32(a) & ToInt32(b));
}

[[nodiscard]] inline Value BitOr(Value a, Value b) noexcept {
    return FromInt32(ToInt32(a) | ToInt32(b));
}

[[nodiscard]] inline Value BitXor(Value a, Value b) noexcept {
    return FromInt32(ToInt32(a) ^ ToInt32(b));
}

[[nodiscard]] inline Value BitNot(Value a) noexcept {
    return FromInt32(~ToInt32(a));
}

// Shift counts outside 0..31 give 0 in both directions.
[[nodiscard]] inline Value Shl(Value a, Value n) noexcept {
    const std::int32_t count = ToInt32(n);
    if(count < 0 || count > 31) {
        return 0.0f;
    }
    const auto pattern = static_cast<std::uint32_t>(ToInt32(a)) << count;
    return FromInt32(std::bit_cast<std::int32_t>(pattern));
}

// Arithmetic: the sign is kept, as in C and JavaScript.
[[nodiscard]] inline Value Shr(Value a, Value n) noexcept {
    const std::int32_t count = ToInt32(n);
    if(count < 0 || count > 31) {
        return 0.0f;
    }
    return FromInt32(ToInt32(a) >> count);
}

} // namespace eerie_leap::expression_engine::detail
