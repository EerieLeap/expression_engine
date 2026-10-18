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

inline constexpr std::size_t MAX_LENGTH = EXPRESSION_ENGINE_MAX_LENGTH;             // characters of text
inline constexpr std::size_t MAX_INSTRUCTIONS = EXPRESSION_ENGINE_MAX_INSTRUCTIONS; // including End
inline constexpr std::size_t MAX_VARIABLES = EXPRESSION_ENGINE_MAX_VARIABLES;       // distinct names
inline constexpr std::size_t MAX_STACK = EXPRESSION_ENGINE_MAX_STACK;               // evaluation stack slots
inline constexpr std::size_t MAX_NESTING = EXPRESSION_ENGINE_MAX_NESTING;           // parser recursion
inline constexpr std::size_t MAX_ARGUMENTS = EXPRESSION_ENGINE_MAX_ARGUMENTS;       // of sum, avg, min, max

static_assert(MAX_LENGTH >= 1 && MAX_LENGTH <= 65535, "positions are 16-bit");
static_assert(MAX_INSTRUCTIONS >= 2 && MAX_INSTRUCTIONS <= 65535, "jump targets are 16-bit");
static_assert(MAX_VARIABLES >= 1 && MAX_VARIABLES <= 65535, "variable indices are 16-bit");
static_assert(MAX_STACK >= 1 && MAX_STACK <= 255, "stack depths are 8-bit");
static_assert(MAX_NESTING >= 1 && MAX_NESTING <= 255, "nesting depths are 8-bit");
static_assert(MAX_ARGUMENTS >= 1 && MAX_ARGUMENTS <= 255, "argument counts are 8-bit");

} // namespace limits

// Per-call bounds for Compile(). Every field defaults to its cap and is clamped to it, so a caller
// can only tighten what the build allows: Compile(text, resource, {.max_instructions = 32}).
struct Limits {
    std::uint16_t max_length = static_cast<std::uint16_t>(limits::MAX_LENGTH);
    std::uint16_t max_instructions = static_cast<std::uint16_t>(limits::MAX_INSTRUCTIONS);
    std::uint16_t max_variables = static_cast<std::uint16_t>(limits::MAX_VARIABLES);
    std::uint8_t max_stack = static_cast<std::uint8_t>(limits::MAX_STACK);
    std::uint8_t max_nesting = static_cast<std::uint8_t>(limits::MAX_NESTING);
    std::uint8_t max_arguments = static_cast<std::uint8_t>(limits::MAX_ARGUMENTS);

    [[nodiscard]] constexpr Limits Clamped() const noexcept {
        return Limits{
            .max_length = std::min(max_length, static_cast<std::uint16_t>(limits::MAX_LENGTH)),
            .max_instructions = std::min(max_instructions, static_cast<std::uint16_t>(limits::MAX_INSTRUCTIONS)),
            .max_variables = std::min(max_variables, static_cast<std::uint16_t>(limits::MAX_VARIABLES)),
            .max_stack = std::min(max_stack, static_cast<std::uint8_t>(limits::MAX_STACK)),
            .max_nesting = std::min(max_nesting, static_cast<std::uint8_t>(limits::MAX_NESTING)),
            .max_arguments = std::min(max_arguments, static_cast<std::uint8_t>(limits::MAX_ARGUMENTS)),
        };
    }

    friend constexpr bool operator==(const Limits&, const Limits&) = default;
};

} // namespace eerie_leap::expression_engine
