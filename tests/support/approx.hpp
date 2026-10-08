#pragma once

#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdlib>

#include <eerie_leap/expression_engine/types.hpp>

namespace eerie_leap::expression_engine::testing {

// Exact comparison: same bits, or both NaN (any payload). Expected values in the vectors are
// computed with the same float operations the engine uses, so this is the normal comparison.
[[nodiscard]] inline bool SameValue(Value expected, Value actual) noexcept {
    if(std::isnan(expected) || std::isnan(actual)) {
        return std::isnan(expected) && std::isnan(actual);
    }
    return std::bit_cast<std::uint32_t>(expected) == std::bit_cast<std::uint32_t>(actual);
}

// Distance in units in the last place, for the few functions whose libm path may differ.
[[nodiscard]] inline std::uint32_t UlpDistance(Value a, Value b) noexcept {
    if(std::isnan(a) || std::isnan(b)) {
        return std::isnan(a) && std::isnan(b) ? 0u : 0xFFFFFFFFu;
    }
    if(a == b) {
        return 0;
    }
    const auto ia = std::bit_cast<std::int32_t>(a);
    const auto ib = std::bit_cast<std::int32_t>(b);
    if((ia < 0) != (ib < 0)) {
        return 0xFFFFFFFFu;
    }
    return static_cast<std::uint32_t>(std::abs(static_cast<std::int64_t>(ia) - static_cast<std::int64_t>(ib)));
}

[[nodiscard]] inline bool WithinUlps(Value expected, Value actual, std::uint32_t ulps) noexcept {
    return ulps == 0 ? SameValue(expected, actual) : UlpDistance(expected, actual) <= ulps;
}

} // namespace eerie_leap::expression_engine::testing
