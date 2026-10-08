#pragma once

#include <cstdint>

namespace eerie_leap::expression_engine {

// Every value an expression computes with. The sensor pipeline stores float, so this is not a
// template parameter.
using Value = float;

// Byte offset into the expression text; see limits::kMaxLength.
using Position = std::uint16_t;

} // namespace eerie_leap::expression_engine
