#include "eerie_leap/expression_engine/error.hpp"

#include <array>
#include <utility>

namespace eerie_leap::expression_engine {

namespace {

constexpr std::array<std::string_view, kCompileErrorCodeCount> DESCRIPTIONS{ {
    "empty expression",
    "expression too long",
    "unexpected character",
    "invalid number",
    "unexpected token",
    "unexpected end of expression",
    "unbalanced parenthesis",
    "unknown function",
    "wrong number of arguments",
    "assignment is not supported",
    "integer literal cannot be represented exactly",
    "too many instructions",
    "too many variables",
    "expression too deep for the evaluation stack",
    "expression nested too deeply",
    "out of memory",
} };

constexpr std::array<std::string_view, kCompileErrorCodeCount> NAMES{ {
    "Empty",
    "TooLong",
    "UnexpectedCharacter",
    "InvalidNumber",
    "UnexpectedToken",
    "UnexpectedEnd",
    "UnbalancedParenthesis",
    "UnknownFunction",
    "WrongArgumentCount",
    "AssignmentNotSupported",
    "IntegerLiteralNotExact",
    "TooManyInstructions",
    "TooManyVariables",
    "StackTooDeep",
    "NestingTooDeep",
    "OutOfMemory",
} };

} // namespace

std::string_view Describe(CompileErrorCode code) noexcept {
    const auto index = std::to_underlying(code);
    return index < DESCRIPTIONS.size() ? DESCRIPTIONS[index] : std::string_view{ "unknown error" };
}

std::string_view Name(CompileErrorCode code) noexcept {
    const auto index = std::to_underlying(code);
    return index < NAMES.size() ? NAMES[index] : std::string_view{ "?" };
}

} // namespace eerie_leap::expression_engine
