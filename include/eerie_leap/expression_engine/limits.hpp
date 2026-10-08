#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>

// Compile-time caps. They size the compiler's scratch buffers and the evaluation stack, so they
// are the hard ceiling of what one build can compile. The host build sets them from CMake cache
// entries and the Zephyr module from Kconfig; a build that sets none gets these defaults.
#ifndef EXPRESSION_ENGINE_MAX_LENGTH
#define EXPRESSION_ENGINE_MAX_LENGTH 256
#endif
#ifndef EXPRESSION_ENGINE_MAX_INSTRUCTIONS
#define EXPRESSION_ENGINE_MAX_INSTRUCTIONS 64
#endif
#ifndef EXPRESSION_ENGINE_MAX_VARIABLES
#define EXPRESSION_ENGINE_MAX_VARIABLES 16
#endif
#ifndef EXPRESSION_ENGINE_MAX_STACK
#define EXPRESSION_ENGINE_MAX_STACK 16
#endif
#ifndef EXPRESSION_ENGINE_MAX_NESTING
#define EXPRESSION_ENGINE_MAX_NESTING 16
#endif
#ifndef EXPRESSION_ENGINE_MAX_ARGUMENTS
#define EXPRESSION_ENGINE_MAX_ARGUMENTS 8
#endif

namespace eerie_leap::expression_engine {

namespace limits {

inline constexpr std::size_t kMaxLength = EXPRESSION_ENGINE_MAX_LENGTH;             // characters of text
inline constexpr std::size_t kMaxInstructions = EXPRESSION_ENGINE_MAX_INSTRUCTIONS; // including End
inline constexpr std::size_t kMaxVariables = EXPRESSION_ENGINE_MAX_VARIABLES;       // distinct names
inline constexpr std::size_t kMaxStack = EXPRESSION_ENGINE_MAX_STACK;               // evaluation stack slots
inline constexpr std::size_t kMaxNesting = EXPRESSION_ENGINE_MAX_NESTING;           // parser recursion
inline constexpr std::size_t kMaxArguments = EXPRESSION_ENGINE_MAX_ARGUMENTS;       // of sum, avg, min, max

static_assert(kMaxLength >= 1 && kMaxLength <= 65535, "positions are 16-bit");
static_assert(kMaxInstructions >= 2 && kMaxInstructions <= 65535, "jump targets are 16-bit");
static_assert(kMaxVariables >= 1 && kMaxVariables <= 65535, "variable indices are 16-bit");
static_assert(kMaxStack >= 1 && kMaxStack <= 255, "stack depths are 8-bit");
static_assert(kMaxNesting >= 1 && kMaxNesting <= 255, "nesting depths are 8-bit");
static_assert(kMaxArguments >= 1 && kMaxArguments <= 255, "argument counts are 8-bit");

} // namespace limits

// Per-call bounds for Compile(). Every field defaults to its cap and is clamped to it, so a caller
// can only tighten what the build allows: Compile(text, resource, {.max_instructions = 32}).
struct Limits {
    std::uint16_t max_length = static_cast<std::uint16_t>(limits::kMaxLength);
    std::uint16_t max_instructions = static_cast<std::uint16_t>(limits::kMaxInstructions);
    std::uint16_t max_variables = static_cast<std::uint16_t>(limits::kMaxVariables);
    std::uint8_t max_stack = static_cast<std::uint8_t>(limits::kMaxStack);
    std::uint8_t max_nesting = static_cast<std::uint8_t>(limits::kMaxNesting);
    std::uint8_t max_arguments = static_cast<std::uint8_t>(limits::kMaxArguments);

    [[nodiscard]] constexpr Limits Clamped() const noexcept {
        return Limits{
            .max_length = std::min(max_length, static_cast<std::uint16_t>(limits::kMaxLength)),
            .max_instructions = std::min(max_instructions, static_cast<std::uint16_t>(limits::kMaxInstructions)),
            .max_variables = std::min(max_variables, static_cast<std::uint16_t>(limits::kMaxVariables)),
            .max_stack = std::min(max_stack, static_cast<std::uint8_t>(limits::kMaxStack)),
            .max_nesting = std::min(max_nesting, static_cast<std::uint8_t>(limits::kMaxNesting)),
            .max_arguments = std::min(max_arguments, static_cast<std::uint8_t>(limits::kMaxArguments)),
        };
    }

    friend constexpr bool operator==(const Limits&, const Limits&) = default;
};

} // namespace eerie_leap::expression_engine
