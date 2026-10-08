#pragma once

// Shared table of compile errors with the position and length Compile() reports for them, at the
// default caps. Consumed by the GoogleTest suite and by tests/zephyr.

#include <array>
#include <string_view>

#include <eerie_leap/expression_engine/error.hpp>

namespace eerie_leap::expression_engine::testing {

struct ErrorVector {
    std::string_view expression;
    CompileError error;
};

inline constexpr std::array<ErrorVector, 42> kErrorVectors{{
    {"", {CompileErrorCode::Empty, 0, 0}},
    {"   \t\n", {CompileErrorCode::Empty, 0, 0}},
    {"1 +", {CompileErrorCode::UnexpectedEnd, 3, 0}},
    {"+", {CompileErrorCode::UnexpectedEnd, 1, 0}},
    {"x +", {CompileErrorCode::UnexpectedEnd, 3, 0}},
    {"x ? 1", {CompileErrorCode::UnexpectedEnd, 5, 0}},
    {"2 * (x", {CompileErrorCode::UnbalancedParenthesis, 4, 1}},
    {"2 * x)", {CompileErrorCode::UnbalancedParenthesis, 5, 1}},
    {"sin(1", {CompileErrorCode::UnbalancedParenthesis, 3, 1}},
    {"(1 2)", {CompileErrorCode::UnexpectedToken, 3, 1}},
    {"1 + * 2", {CompileErrorCode::UnexpectedToken, 4, 1}},
    {"1 2", {CompileErrorCode::UnexpectedToken, 2, 1}},
    {"x ? 1 2", {CompileErrorCode::UnexpectedToken, 6, 1}},
    {"sin(1 2)", {CompileErrorCode::UnexpectedToken, 6, 1}},
    {"sin 1", {CompileErrorCode::UnexpectedToken, 4, 1}},
    {"()", {CompileErrorCode::UnexpectedToken, 1, 1}},
    {"1e", {CompileErrorCode::UnexpectedToken, 1, 1}},
    {"12ab", {CompileErrorCode::UnexpectedToken, 2, 2}},
    {"1.5.3", {CompileErrorCode::UnexpectedToken, 3, 2}},
    {"unknown_fn(x)", {CompileErrorCode::UnknownFunction, 0, 10}},
    {"rnd(1)", {CompileErrorCode::UnknownFunction, 0, 3}},
    {"sin()", {CompileErrorCode::WrongArgumentCount, 0, 3}},
    {"sin(1, 2)", {CompileErrorCode::WrongArgumentCount, 0, 3}},
    {"atan2(1)", {CompileErrorCode::WrongArgumentCount, 0, 5}},
    {"sum()", {CompileErrorCode::WrongArgumentCount, 0, 3}},
    {"sum(1, 2, 3, 4, 5, 6, 7, 8, 9)", {CompileErrorCode::WrongArgumentCount, 0, 3}},
    {"xor(1)", {CompileErrorCode::WrongArgumentCount, 0, 3}},
    {"x = 1", {CompileErrorCode::AssignmentNotSupported, 2, 1}},
    {"sensor_1 = 0", {CompileErrorCode::AssignmentNotSupported, 9, 1}},
    {"a ! b", {CompileErrorCode::UnexpectedCharacter, 2, 1}},
    {"a % b", {CompileErrorCode::UnexpectedCharacter, 2, 1}},
    {"a $ b", {CompileErrorCode::UnexpectedCharacter, 2, 1}},
    {"\"x\"", {CompileErrorCode::UnexpectedCharacter, 0, 1}},
    {"x \xC3\xA9", {CompileErrorCode::UnexpectedCharacter, 2, 1}},
    {".", {CompileErrorCode::InvalidNumber, 0, 1}},
    {"0x", {CompileErrorCode::InvalidNumber, 0, 2}},
    {"0b", {CompileErrorCode::InvalidNumber, 0, 2}},
    {"0x100000000", {CompileErrorCode::InvalidNumber, 0, 11}},
    {"0x1FFFFFF", {CompileErrorCode::IntegerLiteralNotExact, 0, 9}},
    {"0b11111111111111111111111111", {CompileErrorCode::IntegerLiteralNotExact, 0, 28}},
    {"1 + 0b2", {CompileErrorCode::InvalidNumber, 4, 2}},
    {"x & = 1", {CompileErrorCode::AssignmentNotSupported, 4, 1}},
}};

} // namespace eerie_leap::expression_engine::testing
