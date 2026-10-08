#pragma once

#include <expected>
#include <memory_resource>
#include <string_view>

#include "error.hpp"
#include "limits.hpp"
#include "program.hpp"

namespace eerie_leap::expression_engine {

// Compiles an expression into a Program whose storage comes from `resource` (one to three
// allocations: code, variable names, symbols). Never throws: a syntax or limit violation is
// returned as a CompileError with the position of the offending text, and when exceptions are
// enabled an allocation failure becomes CompileErrorCode::OutOfMemory. Compilation itself uses
// the caller's stack only (about limits::kMaxInstructions * 8 bytes of scratch plus the parser's
// recursion, bounded by Limits::max_nesting) and no heap.
//
// `limits` can only tighten the compile-time caps; fields above a cap are clamped to it.
[[nodiscard]] std::expected<Program, CompileError> Compile(
    std::string_view text,
    std::pmr::memory_resource* resource = std::pmr::get_default_resource(),
    const Limits& limits = {}
) noexcept;

} // namespace eerie_leap::expression_engine
