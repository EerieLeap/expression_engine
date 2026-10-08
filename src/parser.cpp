#include "parser.hpp"

#include <array>
#include <numbers>

#include "eerie_leap/expression_engine/functions.hpp"

namespace eerie_leap::expression_engine::detail {

namespace {

// Binary operators by precedence, lowest first; all left-associative. "^" is handled in
// ParsePower because it binds tighter than the unary operators and is right-associative.
struct BinaryOperator {
    TokenKind token;
    std::uint8_t precedence;
    Op op;
};

constexpr std::array<BinaryOperator, 16> kBinaryOperators{{
    {TokenKind::OrOr, 1, Op::OrJump},
    {TokenKind::AndAnd, 2, Op::AndJump},
    {TokenKind::Lt, 3, Op::Lt},
    {TokenKind::Gt, 3, Op::Gt},
    {TokenKind::Le, 3, Op::Le},
    {TokenKind::Ge, 3, Op::Ge},
    {TokenKind::Eq, 3, Op::Eq},
    {TokenKind::Ne, 3, Op::Ne},
    {TokenKind::Pipe, 4, Op::BitOr},
    {TokenKind::Amp, 5, Op::BitAnd},
    {TokenKind::Shl, 6, Op::Shl},
    {TokenKind::Shr, 6, Op::Shr},
    {TokenKind::Plus, 7, Op::Add},
    {TokenKind::Minus, 7, Op::Sub},
    {TokenKind::Star, 8, Op::Mul},
    {TokenKind::Slash, 8, Op::Div},
}};

constexpr const BinaryOperator* FindBinary(TokenKind kind) noexcept {
    for(const BinaryOperator& spec : kBinaryOperators) {
        if(spec.token == kind) {
            return &spec;
        }
    }
    return nullptr;
}

constexpr CompileError At(CompileErrorCode code, const Token& token) noexcept {
    return CompileError{code, token.position, token.length};
}

// Attaches the position of `token` to an emitter failure.
constexpr Parser::Result Lift(std::expected<void, CompileErrorCode> result, const Token& token) noexcept {
    if(!result) {
        return std::unexpected(At(result.error(), token));
    }
    return {};
}

} // namespace

Parser::Parser(Lexer& lexer, Emitter& emitter, const Limits& limits) noexcept
    : lexer_(lexer), emitter_(emitter), limits_(limits) {}

Parser::Result Parser::Enter(const Token& at) noexcept {
    if(depth_ >= limits_.max_nesting) {
        return std::unexpected(At(CompileErrorCode::NestingTooDeep, at));
    }
    ++depth_;
    return {};
}

void Parser::Leave() noexcept {
    --depth_;
}

Parser::Result Parser::Parse() noexcept {
    if(const auto r = ParseTernary(); !r) {
        return r;
    }

    const auto token = lexer_.Next();
    if(!token) {
        return std::unexpected(token.error());
    }
    if(token->kind == TokenKind::End) {
        return {};
    }
    if(token->kind == TokenKind::RParen) {
        return std::unexpected(At(CompileErrorCode::UnbalancedParenthesis, *token));
    }
    return std::unexpected(At(CompileErrorCode::UnexpectedToken, *token));
}

Parser::Result Parser::ParseTernary() noexcept {
    if(const auto r = ParseBinary(1); !r) {
        return r;
    }

    const auto question = lexer_.Peek();
    if(!question) {
        return std::unexpected(question.error());
    }
    if(question->kind != TokenKind::Question) {
        return {};
    }
    (void)lexer_.Next();

    if(const auto r = Enter(*question); !r) {
        return r;
    }

    const auto skip_true = emitter_.EmitJump(Op::JumpIfFalse);
    if(!skip_true) {
        return std::unexpected(At(skip_true.error(), *question));
    }

    if(const auto r = ParseTernary(); !r) {
        return r;
    }

    const auto colon = lexer_.Next();
    if(!colon) {
        return std::unexpected(colon.error());
    }
    if(colon->kind != TokenKind::Colon) {
        return std::unexpected(
            At(colon->kind == TokenKind::End ? CompileErrorCode::UnexpectedEnd : CompileErrorCode::UnexpectedToken,
               *colon)
        );
    }

    const auto skip_false = emitter_.EmitJump(Op::Jump);
    if(!skip_false) {
        return std::unexpected(At(skip_false.error(), *colon));
    }
    if(const auto r = Lift(emitter_.PatchJump(*skip_true), *colon); !r) {
        return r;
    }

    if(const auto r = ParseTernary(); !r) {
        return r;
    }

    if(const auto r = Lift(emitter_.PatchJump(*skip_false), *colon); !r) {
        return r;
    }

    Leave();
    return {};
}

Parser::Result Parser::ParseBinary(std::uint8_t min_precedence) noexcept {
    if(const auto r = ParseUnary(); !r) {
        return r;
    }

    for(;;) {
        const auto token = lexer_.Peek();
        if(!token) {
            return std::unexpected(token.error());
        }

        const BinaryOperator* spec = FindBinary(token->kind);
        if(spec == nullptr || spec->precedence < min_precedence) {
            return {};
        }
        (void)lexer_.Next();

        const auto next_precedence = static_cast<std::uint8_t>(spec->precedence + 1);

        if(spec->op == Op::AndJump || spec->op == Op::OrJump) {
            const auto jump = emitter_.EmitJump(spec->op);
            if(!jump) {
                return std::unexpected(At(jump.error(), *token));
            }
            if(const auto r = ParseBinary(next_precedence); !r) {
                return r;
            }
            if(const auto r = Lift(emitter_.EmitUnary(Op::ToBool), *token); !r) {
                return r;
            }
            if(const auto r = Lift(emitter_.PatchJump(*jump), *token); !r) {
                return r;
            }
            continue;
        }

        if(const auto r = ParseBinary(next_precedence); !r) {
            return r;
        }
        if(const auto r = Lift(emitter_.EmitBinary(spec->op), *token); !r) {
            return r;
        }
    }
}

Parser::Result Parser::ParseUnary() noexcept {
    const auto token = lexer_.Peek();
    if(!token) {
        return std::unexpected(token.error());
    }

    if(token->kind != TokenKind::Minus && token->kind != TokenKind::Plus && token->kind != TokenKind::Tilde) {
        return ParsePower();
    }
    (void)lexer_.Next();

    if(const auto r = Enter(*token); !r) {
        return r;
    }
    if(const auto r = ParseUnary(); !r) {
        return r;
    }
    Leave();

    switch(token->kind) {
    case TokenKind::Minus:
        return Lift(emitter_.EmitUnary(Op::Neg), *token);
    case TokenKind::Tilde:
        return Lift(emitter_.EmitUnary(Op::BitNot), *token);
    default:
        return {};
    }
}

Parser::Result Parser::ParsePower() noexcept {
    if(const auto r = ParsePrimary(); !r) {
        return r;
    }

    const auto token = lexer_.Peek();
    if(!token) {
        return std::unexpected(token.error());
    }
    if(token->kind != TokenKind::Caret) {
        return {};
    }
    (void)lexer_.Next();

    // The exponent may carry its own sign ("2^-1") and its own "^" (right-associative).
    if(const auto r = Enter(*token); !r) {
        return r;
    }
    if(const auto r = ParseUnary(); !r) {
        return r;
    }
    Leave();

    return Lift(emitter_.EmitBinary(Op::Pow), *token);
}

Parser::Result Parser::ParsePrimary() noexcept {
    const auto token = lexer_.Next();
    if(!token) {
        return std::unexpected(token.error());
    }

    switch(token->kind) {
    case TokenKind::Number:
        return Lift(emitter_.EmitConst(token->value), *token);

    case TokenKind::Identifier: {
        const auto next = lexer_.Peek();
        if(!next) {
            return std::unexpected(next.error());
        }
        if(next->kind == TokenKind::LParen) {
            return ParseCall(*token);
        }

        const std::string_view name = lexer_.Text(*token);
        if(name == "_pi") {
            return Lift(emitter_.EmitConst(std::numbers::pi_v<Value>), *token);
        }
        if(name == "_e") {
            return Lift(emitter_.EmitConst(std::numbers::e_v<Value>), *token);
        }
        return Lift(emitter_.EmitVariable(name), *token);
    }

    case TokenKind::LParen: {
        if(const auto r = Enter(*token); !r) {
            return r;
        }
        if(const auto r = ParseTernary(); !r) {
            return r;
        }
        const auto close = lexer_.Next();
        if(!close) {
            return std::unexpected(close.error());
        }
        if(close->kind == TokenKind::End) {
            return std::unexpected(At(CompileErrorCode::UnbalancedParenthesis, *token));
        }
        if(close->kind != TokenKind::RParen) {
            return std::unexpected(At(CompileErrorCode::UnexpectedToken, *close));
        }
        Leave();
        return {};
    }

    case TokenKind::End:
        return std::unexpected(At(CompileErrorCode::UnexpectedEnd, *token));

    default:
        return std::unexpected(At(CompileErrorCode::UnexpectedToken, *token));
    }
}

Parser::Result Parser::ParseCall(const Token& name) noexcept {
    const FunctionSpec* spec = FindFunction(lexer_.Text(name));
    if(spec == nullptr) {
        return std::unexpected(At(CompileErrorCode::UnknownFunction, name));
    }

    const auto open = lexer_.Next(); // the "(" the caller peeked
    if(!open) {
        return std::unexpected(open.error());
    }
    if(const auto r = Enter(name); !r) {
        return r;
    }

    const std::uint8_t max_args = spec->max_args == kVariadic ? limits_.max_arguments : spec->max_args;
    std::uint8_t argc = 0;

    const auto first = lexer_.Peek();
    if(!first) {
        return std::unexpected(first.error());
    }

    if(first->kind == TokenKind::RParen) {
        (void)lexer_.Next();
    } else {
        for(;;) {
            if(argc >= max_args) {
                return std::unexpected(At(CompileErrorCode::WrongArgumentCount, name));
            }
            if(const auto r = ParseTernary(); !r) {
                return r;
            }
            ++argc;

            const auto separator = lexer_.Next();
            if(!separator) {
                return std::unexpected(separator.error());
            }
            if(separator->kind == TokenKind::RParen) {
                break;
            }
            if(separator->kind == TokenKind::Comma) {
                continue;
            }
            if(separator->kind == TokenKind::End) {
                return std::unexpected(At(CompileErrorCode::UnbalancedParenthesis, *open));
            }
            return std::unexpected(At(CompileErrorCode::UnexpectedToken, *separator));
        }
    }

    if(argc < spec->min_args) {
        return std::unexpected(At(CompileErrorCode::WrongArgumentCount, name));
    }

    if(const auto r = Lift(emitter_.EmitCall(spec->id, argc), name); !r) {
        return r;
    }
    Leave();
    return {};
}

} // namespace eerie_leap::expression_engine::detail
