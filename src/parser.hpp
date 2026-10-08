#pragma once

#include <cstdint>
#include <expected>

#include "eerie_leap/expression_engine/error.hpp"
#include "eerie_leap/expression_engine/limits.hpp"

#include "emitter.hpp"
#include "lexer.hpp"

namespace eerie_leap::expression_engine::detail {

// Precedence-climbing parser that drives the emitter directly; it never sees an Instruction.
// Recursion is bounded by Limits::max_nesting, counted at every construct that nests: "(",
// a call, a ternary, a unary operator and the exponent of "^".
class Parser {
public:
    using Result = std::expected<void, CompileError>;

    Parser(Lexer& lexer, Emitter& emitter, const Limits& limits) noexcept;

    // Parses the whole text as one expression.
    [[nodiscard]] Result Parse() noexcept;

private:
    [[nodiscard]] Result ParseTernary() noexcept;
    [[nodiscard]] Result ParseBinary(std::uint8_t min_precedence) noexcept;
    [[nodiscard]] Result ParseUnary() noexcept;
    [[nodiscard]] Result ParsePower() noexcept;
    [[nodiscard]] Result ParsePrimary() noexcept;
    [[nodiscard]] Result ParseCall(const Token& name) noexcept;
    [[nodiscard]] Result Enter(const Token& at) noexcept;
    void Leave() noexcept;

    Lexer& lexer_;
    Emitter& emitter_;
    Limits limits_;
    std::uint8_t depth_ = 0;
};

} // namespace eerie_leap::expression_engine::detail
