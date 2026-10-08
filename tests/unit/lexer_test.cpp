#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "lexer.hpp"
#include "support/printers.hpp"

namespace eerie_leap::expression_engine::detail {

namespace {

std::vector<Token> Tokenize(std::string_view text) {
    Lexer lexer(text);
    std::vector<Token> tokens;
    for(;;) {
        const auto token = lexer.Next();
        if(!token) {
            ADD_FAILURE() << "lexer error " << Name(token.error().code) << " at " << token.error().position;
            return tokens;
        }
        tokens.push_back(*token);
        if(token->kind == TokenKind::End) {
            return tokens;
        }
    }
}

CompileError FirstError(std::string_view text) {
    Lexer lexer(text);
    for(;;) {
        const auto token = lexer.Next();
        if(!token) {
            return token.error();
        }
        if(token->kind == TokenKind::End) {
            ADD_FAILURE() << "no lexer error in \"" << text << "\"";
            return CompileError{CompileErrorCode::Empty, 0, 0};
        }
    }
}

} // namespace

TEST(Lexer, RecognizesEveryTokenKind) {
    const auto tokens = Tokenize("( ) , ? : + - * / ^ < > <= >= == != && || & | << >> ~ 1 x");
    const std::vector<TokenKind> expected{
        TokenKind::LParen, TokenKind::RParen, TokenKind::Comma, TokenKind::Question, TokenKind::Colon,
        TokenKind::Plus,   TokenKind::Minus,  TokenKind::Star,  TokenKind::Slash,    TokenKind::Caret,
        TokenKind::Lt,     TokenKind::Gt,     TokenKind::Le,    TokenKind::Ge,       TokenKind::Eq,
        TokenKind::Ne,     TokenKind::AndAnd, TokenKind::OrOr,  TokenKind::Amp,      TokenKind::Pipe,
        TokenKind::Shl,    TokenKind::Shr,    TokenKind::Tilde, TokenKind::Number,   TokenKind::Identifier,
        TokenKind::End,
    };
    ASSERT_EQ(tokens.size(), expected.size());
    for(std::size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(tokens[i].kind, expected[i]) << "token " << i;
    }
    EXPECT_EQ(tokens[12].position, 24); // "<="
    EXPECT_EQ(tokens[12].length, 2);
    EXPECT_EQ(tokens[25].position, 57); // End sits at the end of the text
}

TEST(Lexer, UsesLongestMatchForTwoCharacterOperators) {
    EXPECT_EQ(Tokenize("a<<b")[1].kind, TokenKind::Shl);
    EXPECT_EQ(Tokenize("a<=b")[1].kind, TokenKind::Le);
    EXPECT_EQ(Tokenize("a<b")[1].kind, TokenKind::Lt);
    EXPECT_EQ(Tokenize("a&&b")[1].kind, TokenKind::AndAnd);
    EXPECT_EQ(Tokenize("a&b")[1].kind, TokenKind::Amp);
    EXPECT_EQ(Tokenize("a||b")[1].kind, TokenKind::OrOr);
    EXPECT_EQ(Tokenize("a|b")[1].kind, TokenKind::Pipe);
    EXPECT_EQ(Tokenize("a>>b")[1].kind, TokenKind::Shr);
    EXPECT_EQ(Tokenize("a!=b")[1].kind, TokenKind::Ne);
    EXPECT_EQ(Tokenize("a==b")[1].kind, TokenKind::Eq);

    // "<<=" is "<<" followed by an assignment.
    const CompileError error = FirstError("a <<= b");
    EXPECT_EQ(error, (CompileError{CompileErrorCode::AssignmentNotSupported, 4, 1}));
}

TEST(Lexer, ParsesDecimalNumbers) {
    struct Case {
        std::string_view text;
        Value value;
        std::uint16_t length;
    };
    const Case cases[]{
        {"1", 1.0f, 1},
        {"1.", 1.0f, 2},
        {".5", 0.5f, 2},
        {"1e3", 1000.0f, 3},
        {"1E-3", 0.001f, 4},
        {"2.5e+2", 250.0f, 6},
        {"007", 7.0f, 3},
        {"3.1415927", 3.1415927f, 9},
    };
    for(const Case& c : cases) {
        const auto tokens = Tokenize(c.text);
        ASSERT_EQ(tokens.size(), 2u) << c.text;
        EXPECT_EQ(tokens[0].kind, TokenKind::Number) << c.text;
        EXPECT_EQ(tokens[0].value, c.value) << c.text;
        EXPECT_EQ(tokens[0].length, c.length) << c.text;
    }
}

TEST(Lexer, ParsesIntegerLiterals) {
    struct Case {
        std::string_view text;
        Value value;
    };
    const Case cases[]{
        {"0xFF", 255.0f},
        {"0XfF", 255.0f},
        {"0x0", 0.0f},
        {"0b1010", 10.0f},
        {"0B11", 3.0f},
        {"0xFFFFFFFF", -1.0f},
        {"0x80000000", -2147483648.0f},
        {"0x00FFFFFF", 16777215.0f},
        {"0xFFFF0000", -65536.0f},
    };
    for(const Case& c : cases) {
        const auto tokens = Tokenize(c.text);
        ASSERT_EQ(tokens.size(), 2u) << c.text;
        EXPECT_EQ(tokens[0].kind, TokenKind::Number) << c.text;
        EXPECT_EQ(tokens[0].value, c.value) << c.text;
        EXPECT_EQ(tokens[0].length, c.text.size()) << c.text;
    }
}

TEST(Lexer, RejectsBadIntegerLiterals) {
    EXPECT_EQ(FirstError("0x"), (CompileError{CompileErrorCode::InvalidNumber, 0, 2}));
    EXPECT_EQ(FirstError("0b"), (CompileError{CompileErrorCode::InvalidNumber, 0, 2}));
    EXPECT_EQ(FirstError("0xG"), (CompileError{CompileErrorCode::InvalidNumber, 0, 2}));
    EXPECT_EQ(FirstError("0x100000000"), (CompileError{CompileErrorCode::InvalidNumber, 0, 11}));
    EXPECT_EQ(FirstError("0x1FFFFFF"), (CompileError{CompileErrorCode::IntegerLiteralNotExact, 0, 9}));
    EXPECT_EQ(
        FirstError("x + 0b11111111111111111111111111"),
        (CompileError{CompileErrorCode::IntegerLiteralNotExact, 4, 28})
    );
}

TEST(Lexer, LeavesTrailingLettersAsIdentifiers) {
    auto tokens = Tokenize("1e");
    ASSERT_EQ(tokens.size(), 3u);
    EXPECT_EQ(tokens[0].kind, TokenKind::Number);
    EXPECT_EQ(tokens[0].length, 1);
    EXPECT_EQ(tokens[1].kind, TokenKind::Identifier);
    EXPECT_EQ(tokens[1].position, 1);

    tokens = Tokenize("12ab");
    ASSERT_EQ(tokens.size(), 3u);
    EXPECT_EQ(tokens[0].value, 12.0f);
    EXPECT_EQ(tokens[1].kind, TokenKind::Identifier);
    EXPECT_EQ(tokens[1].position, 2);
    EXPECT_EQ(tokens[1].length, 2);

    tokens = Tokenize("0x10g");
    ASSERT_EQ(tokens.size(), 3u);
    EXPECT_EQ(tokens[0].value, 16.0f);
    EXPECT_EQ(tokens[1].kind, TokenKind::Identifier);
}

TEST(Lexer, ScansIdentifiers) {
    const std::string_view text = "_pi x1 sensor_1 Ab_9";
    Lexer lexer(text);
    const std::string_view expected[]{"_pi", "x1", "sensor_1", "Ab_9"};
    for(const std::string_view name : expected) {
        const auto token = lexer.Next();
        ASSERT_TRUE(token);
        EXPECT_EQ(token->kind, TokenKind::Identifier);
        EXPECT_EQ(lexer.Text(*token), name);
    }
    EXPECT_EQ(lexer.Next()->kind, TokenKind::End);
}

TEST(Lexer, ReportsCharactersThatStartNoToken) {
    EXPECT_EQ(FirstError("="), (CompileError{CompileErrorCode::AssignmentNotSupported, 0, 1}));
    EXPECT_EQ(FirstError("!"), (CompileError{CompileErrorCode::UnexpectedCharacter, 0, 1}));
    EXPECT_EQ(FirstError("x % 2"), (CompileError{CompileErrorCode::UnexpectedCharacter, 2, 1}));
    EXPECT_EQ(FirstError("$"), (CompileError{CompileErrorCode::UnexpectedCharacter, 0, 1}));
    EXPECT_EQ(FirstError("\xC3\xA9"), (CompileError{CompileErrorCode::UnexpectedCharacter, 0, 1}));
    EXPECT_EQ(FirstError("'x'"), (CompileError{CompileErrorCode::UnexpectedCharacter, 0, 1}));
    EXPECT_EQ(FirstError("."), (CompileError{CompileErrorCode::InvalidNumber, 0, 1}));
    EXPECT_EQ(FirstError("1e400"), (CompileError{CompileErrorCode::InvalidNumber, 0, 5}));
}

TEST(Lexer, PeekDoesNotConsume) {
    Lexer lexer("1 + 2");
    const auto first = lexer.Peek();
    const auto again = lexer.Peek();
    ASSERT_TRUE(first && again);
    EXPECT_EQ(first->position, again->position);
    EXPECT_EQ(lexer.Next()->kind, TokenKind::Number);
    EXPECT_EQ(lexer.Peek()->kind, TokenKind::Plus);
    EXPECT_EQ(lexer.Next()->kind, TokenKind::Plus);
    EXPECT_EQ(lexer.Next()->kind, TokenKind::Number);
    EXPECT_EQ(lexer.Next()->kind, TokenKind::End);
    EXPECT_EQ(lexer.Next()->kind, TokenKind::End);
}

TEST(Lexer, SkipsEveryKindOfWhitespace) {
    const auto tokens = Tokenize(" \t\n\r\v\f1");
    ASSERT_EQ(tokens.size(), 2u);
    EXPECT_EQ(tokens[0].position, 6);
}

} // namespace eerie_leap::expression_engine::detail
