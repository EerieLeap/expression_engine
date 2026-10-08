#include "lexer.hpp"

#include <bit>
#include <charconv>
#include <cstdint>

#include "bit_ops.hpp"

namespace eerie_leap::expression_engine::detail {

namespace {

// ASCII only, on purpose: <cctype> is locale-dependent and undefined for negative char.
constexpr bool IsDigit(char c) noexcept {
    return c >= '0' && c <= '9';
}

constexpr bool IsAlpha(char c) noexcept {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

constexpr bool IsIdentifierStart(char c) noexcept {
    return IsAlpha(c) || c == '_';
}

constexpr bool IsIdentifierPart(char c) noexcept {
    return IsIdentifierStart(c) || IsDigit(c);
}

constexpr bool IsSpace(char c) noexcept {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
}

constexpr bool IsDigitOf(int base, char c) noexcept {
    if(base == 2) {
        return c == '0' || c == '1';
    }
    return IsDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

constexpr std::expected<Token, CompileError> Fail(
    CompileErrorCode code,
    Position position,
    std::uint16_t length
) noexcept {
    return std::unexpected(CompileError{code, position, length});
}

} // namespace

std::expected<Token, CompileError> Lexer::Peek() noexcept {
    if(!peeked_) {
        peeked_ = Scan();
    }
    return *peeked_;
}

std::expected<Token, CompileError> Lexer::Next() noexcept {
    if(peeked_) {
        const auto token = *peeked_;
        peeked_.reset();
        return token;
    }
    return Scan();
}

char Lexer::At(std::size_t offset) const noexcept {
    const std::size_t index = position_ + offset;
    return index < text_.size() ? text_[index] : '\0';
}

Token Lexer::Single(TokenKind kind) noexcept {
    const Token token{kind, position_, 1, 0.0f};
    ++position_;
    return token;
}

Token Lexer::Double(TokenKind kind) noexcept {
    const Token token{kind, position_, 2, 0.0f};
    position_ = static_cast<Position>(position_ + 2);
    return token;
}

std::expected<Token, CompileError> Lexer::Scan() noexcept {
    while(position_ < text_.size() && IsSpace(text_[position_])) {
        ++position_;
    }

    if(position_ >= text_.size()) {
        return Token{TokenKind::End, position_, 0, 0.0f};
    }

    const char c = text_[position_];

    if(IsDigit(c) || c == '.') {
        return ScanNumber();
    }
    if(IsIdentifierStart(c)) {
        return ScanIdentifier();
    }

    switch(c) {
    case '(':
        return Single(TokenKind::LParen);
    case ')':
        return Single(TokenKind::RParen);
    case ',':
        return Single(TokenKind::Comma);
    case '?':
        return Single(TokenKind::Question);
    case ':':
        return Single(TokenKind::Colon);
    case '+':
        return Single(TokenKind::Plus);
    case '-':
        return Single(TokenKind::Minus);
    case '*':
        return Single(TokenKind::Star);
    case '/':
        return Single(TokenKind::Slash);
    case '^':
        return Single(TokenKind::Caret);
    case '~':
        return Single(TokenKind::Tilde);
    case '<':
        if(At(1) == '=') {
            return Double(TokenKind::Le);
        }
        if(At(1) == '<') {
            return Double(TokenKind::Shl);
        }
        return Single(TokenKind::Lt);
    case '>':
        if(At(1) == '=') {
            return Double(TokenKind::Ge);
        }
        if(At(1) == '>') {
            return Double(TokenKind::Shr);
        }
        return Single(TokenKind::Gt);
    case '=':
        if(At(1) == '=') {
            return Double(TokenKind::Eq);
        }
        return Fail(CompileErrorCode::AssignmentNotSupported, position_, 1);
    case '!':
        if(At(1) == '=') {
            return Double(TokenKind::Ne);
        }
        return Fail(CompileErrorCode::UnexpectedCharacter, position_, 1);
    case '&':
        if(At(1) == '&') {
            return Double(TokenKind::AndAnd);
        }
        return Single(TokenKind::Amp);
    case '|':
        if(At(1) == '|') {
            return Double(TokenKind::OrOr);
        }
        return Single(TokenKind::Pipe);
    default:
        return Fail(CompileErrorCode::UnexpectedCharacter, position_, 1);
    }
}

std::expected<Token, CompileError> Lexer::ScanNumber() noexcept {
    const Position start = position_;

    if(text_[start] == '0') {
        const char prefix = At(1);
        if(prefix == 'x' || prefix == 'X') {
            return ScanIntegerLiteral(16);
        }
        if(prefix == 'b' || prefix == 'B') {
            return ScanIntegerLiteral(2);
        }
    }

    const char* const begin = text_.data() + start;
    const char* const end = text_.data() + text_.size();
    Value value = 0.0f;
    const auto [ptr, ec] = std::from_chars(begin, end, value, std::chars_format::general);
    const auto length = static_cast<std::uint16_t>(ptr - begin);

    if(ec != std::errc{}) {
        return Fail(CompileErrorCode::InvalidNumber, start, length == 0 ? std::uint16_t{1} : length);
    }

    position_ = static_cast<Position>(start + length);
    return Token{TokenKind::Number, start, length, value};
}

// "0x" or "0b" prefix: a 32-bit pattern read as a signed integer, which must survive the round
// trip through float (docs/LANGUAGE.md, "Integer literals").
std::expected<Token, CompileError> Lexer::ScanIntegerLiteral(int base) noexcept {
    const Position start = position_;
    const std::size_t digits_begin = static_cast<std::size_t>(start) + 2;
    std::size_t digits_end = digits_begin;
    while(digits_end < text_.size() && IsDigitOf(base, text_[digits_end])) {
        ++digits_end;
    }
    const auto length = static_cast<std::uint16_t>(digits_end - start);

    if(digits_end == digits_begin) {
        return Fail(CompileErrorCode::InvalidNumber, start, 2);
    }

    std::uint32_t pattern = 0;
    const auto [ptr, ec] = std::from_chars(text_.data() + digits_begin, text_.data() + digits_end, pattern, base);
    if(ec != std::errc{}) {
        return Fail(CompileErrorCode::InvalidNumber, start, length);
    }

    const auto integer = std::bit_cast<std::int32_t>(pattern);
    const Value value = FromInt32(integer);
    if(ToInt32(value) != integer) {
        return Fail(CompileErrorCode::IntegerLiteralNotExact, start, length);
    }

    position_ = static_cast<Position>(digits_end);
    return Token{TokenKind::Number, start, length, value};
}

Token Lexer::ScanIdentifier() noexcept {
    const Position start = position_;
    while(position_ < text_.size() && IsIdentifierPart(text_[position_])) {
        ++position_;
    }
    return Token{TokenKind::Identifier, start, static_cast<std::uint16_t>(position_ - start), 0.0f};
}

} // namespace eerie_leap::expression_engine::detail
