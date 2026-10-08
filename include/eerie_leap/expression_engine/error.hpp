#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "types.hpp"

namespace eerie_leap::expression_engine {

enum class CompileErrorCode : std::uint8_t {
    Empty,                  // nothing but whitespace
    TooLong,                // text longer than max_length
    UnexpectedCharacter,    // a character that starts no token
    InvalidNumber,          // a number that std::from_chars rejects, or an integer literal that overflows
    UnexpectedToken,        // a token where another was expected
    UnexpectedEnd,          // the text ends inside an expression
    UnbalancedParenthesis,  // a "(" without ")" or a ")" without "("
    UnknownFunction,        // name followed by "(" that is not in kFunctions
    WrongArgumentCount,     // outside the function's range or above max_arguments
    AssignmentNotSupported, // "=": the language has no assignment; its own code so the message says so
    IntegerLiteralNotExact, // a hex or binary literal that float cannot hold exactly
    TooManyInstructions,    // above max_instructions
    TooManyVariables,       // above max_variables distinct names
    StackTooDeep,           // the evaluation stack would exceed max_stack
    NestingTooDeep,         // parser recursion would exceed max_nesting
    OutOfMemory,            // the memory resource failed (only when exceptions are enabled)
};

inline constexpr std::size_t kCompileErrorCodeCount = 16;

struct CompileError {
    CompileErrorCode code;
    Position position;    // byte offset of the offending text
    std::uint16_t length; // of the offending token, 0 at end of input

    friend constexpr bool operator==(const CompileError&, const CompileError&) = default;
};

// Human-readable description, e.g. "unknown function".
[[nodiscard]] std::string_view Describe(CompileErrorCode code) noexcept;

// The enumerator's name, e.g. "UnknownFunction".
[[nodiscard]] std::string_view Name(CompileErrorCode code) noexcept;

} // namespace eerie_leap::expression_engine
