#pragma once

#include <cstdint>
#include <expected>
#include <optional>
#include <string_view>

#include "eerie_leap/expression_engine/error.hpp"
#include "eerie_leap/expression_engine/types.hpp"

namespace eerie_leap::expression_engine::detail {

enum class TokenKind : std::uint8_t {
    End,
    Number,
    Identifier,
    LParen,
    RParen,
    Comma,
    Question,
    Colon,
    Plus,
    Minus,
    Star,
    Slash,
    Caret,
    Lt,
    Gt,
    Le,
    Ge,
    Eq,
    Ne,
    AndAnd,
    OrOr,
    Amp,
    Pipe,
    Shl,
    Shr,
    Tilde,
};

struct Token {
    TokenKind kind;
    Position position;
    std::uint16_t length;
    Value value; // Number only
};

// Pull-based tokenizer over the expression text: no token buffer, no allocation. The text is at
// most 65535 bytes (Compile checks max_length first) so positions fit a Position.
class Lexer {
public:
    explicit Lexer(std::string_view text) noexcept : text_(text) {}

    // The next token without consuming it.
    [[nodiscard]] std::expected<Token, CompileError> Peek() noexcept;

    // Consumes and returns the next token. At the end of the text it keeps returning End.
    [[nodiscard]] std::expected<Token, CompileError> Next() noexcept;

    [[nodiscard]] std::string_view Text(const Token& token) const noexcept {
        return text_.substr(token.position, token.length);
    }

private:
    [[nodiscard]] std::expected<Token, CompileError> Scan() noexcept;
    [[nodiscard]] std::expected<Token, CompileError> ScanNumber() noexcept;
    [[nodiscard]] std::expected<Token, CompileError> ScanIntegerLiteral(int base) noexcept;
    [[nodiscard]] Token ScanIdentifier() noexcept;
    [[nodiscard]] Token Single(TokenKind kind) noexcept;
    [[nodiscard]] Token Double(TokenKind kind) noexcept;
    [[nodiscard]] char At(std::size_t offset) const noexcept;

    std::string_view text_;
    Position position_ = 0;
    std::optional<std::expected<Token, CompileError>> peeked_;
};

} // namespace eerie_leap::expression_engine::detail
