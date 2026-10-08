#pragma once

#include <cstdint>
#include <version>

#if !defined(__cpp_lib_expected)
#error "eerie_leap_expression_engine needs std::expected: GCC 12+, or Clang 19+ with libstdc++ 13+"
#endif

namespace eerie_leap::expression_engine {

// Every value an expression computes with. The sensor pipeline stores float, so this is not a
// template parameter.
using Value = float;

// Byte offset into the expression text; see limits::kMaxLength.
using Position = std::uint16_t;

} // namespace eerie_leap::expression_engine
