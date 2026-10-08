#pragma once

#include <cstddef>

namespace eerie_leap::expression_engine::testing {

// Number of calls to the global operator new in this test binary (allocation_counter.cpp).
[[nodiscard]] std::size_t GlobalAllocations() noexcept;

} // namespace eerie_leap::expression_engine::testing
