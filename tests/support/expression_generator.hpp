#pragma once

// Random expression trees for the property test. Each tree is rendered twice, fully parenthesized
// and with only the parentheses the precedence table requires, and its value is computed while it
// is built with the same float operations the engine uses, so the engine must reproduce it bit
// for bit from either rendering.

#include <array>
#include <cmath>
#include <cstdint>
#include <random>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <eerie_leap/expression_engine/functions.hpp>
#include <eerie_leap/expression_engine/types.hpp>

#include "bit_ops.hpp"
#include "vectors/evaluation_vectors.hpp"

namespace eerie_leap::expression_engine::testing {

struct GeneratedExpression {
    std::string full;
    std::string minimal;
    Value value;
    int precedence; // of the outermost construct
};

class ExpressionGenerator {
public:
    // Precedence levels of the grammar, lowest first; see docs/LANGUAGE.md.
    static constexpr int kTernary = 0;
    static constexpr int kOr = 1;
    static constexpr int kAnd = 2;
    static constexpr int kCompare = 3;
    static constexpr int kBitOr = 4;
    static constexpr int kBitAnd = 5;
    static constexpr int kShift = 6;
    static constexpr int kAdd = 7;
    static constexpr int kMul = 8;
    static constexpr int kUnary = 9;
    static constexpr int kPow = 10;
    static constexpr int kPrimary = 11;

    ExpressionGenerator(std::uint32_t seed, std::span<const Binding> variables) : rng_(seed), variables_(variables) {}

    GeneratedExpression Generate(int depth) {
        if(depth <= 0) {
            return Leaf();
        }
        const int choice = Pick(20);
        if(choice < 10) {
            return Binary(depth);
        }
        if(choice < 12) {
            return Unary(depth);
        }
        if(choice < 14) {
            return Ternary(depth);
        }
        if(choice < 17) {
            return Call(depth);
        }
        return Leaf();
    }

private:
    struct ConstantSpec {
        std::string_view text;
        Value value;
    };

    struct OperatorSpec {
        std::string_view text;
        int precedence;
    };

    static constexpr std::array<ConstantSpec, 16> kConstants{{
        {"0", 0.0f},
        {"1", 1.0f},
        {"2", 2.0f},
        {"3", 3.0f},
        {"4", 4.0f},
        {"7", 7.0f},
        {"10", 10.0f},
        {"255", 255.0f},
        {"0.5", 0.5f},
        {"1.5", 1.5f},
        {"2.5", 2.5f},
        {"0.1", 0.1f},
        {"1e3", 1000.0f},
        {"0xFF", 255.0f},
        {"0x10", 16.0f},
        {"0b101", 5.0f},
    }};

    static constexpr std::array<OperatorSpec, 17> kOperators{{
        {"||", kOr},
        {"&&", kAnd},
        {"<", kCompare},
        {">", kCompare},
        {"<=", kCompare},
        {">=", kCompare},
        {"==", kCompare},
        {"!=", kCompare},
        {"|", kBitOr},
        {"&", kBitAnd},
        {"<<", kShift},
        {">>", kShift},
        {"+", kAdd},
        {"-", kAdd},
        {"*", kMul},
        {"/", kMul},
        {"^", kPow},
    }};

    int Pick(int count) {
        return std::uniform_int_distribution<int>(0, count - 1)(rng_);
    }

    static std::string Wrap(const std::string& text) {
        return "(" + text + ")";
    }

    static Value Apply(std::string_view op, Value a, Value b) {
        if(op == "||") {
            return (a != 0.0f || b != 0.0f) ? 1.0f : 0.0f;
        }
        if(op == "&&") {
            return (a != 0.0f && b != 0.0f) ? 1.0f : 0.0f;
        }
        if(op == "<") {
            return a < b ? 1.0f : 0.0f;
        }
        if(op == ">") {
            return a > b ? 1.0f : 0.0f;
        }
        if(op == "<=") {
            return a <= b ? 1.0f : 0.0f;
        }
        if(op == ">=") {
            return a >= b ? 1.0f : 0.0f;
        }
        if(op == "==") {
            return a == b ? 1.0f : 0.0f;
        }
        if(op == "!=") {
            return a != b ? 1.0f : 0.0f;
        }
        if(op == "|") {
            return detail::BitOr(a, b);
        }
        if(op == "&") {
            return detail::BitAnd(a, b);
        }
        if(op == "<<") {
            return detail::Shl(a, b);
        }
        if(op == ">>") {
            return detail::Shr(a, b);
        }
        if(op == "+") {
            return a + b;
        }
        if(op == "-") {
            return a - b;
        }
        if(op == "*") {
            return a * b;
        }
        if(op == "/") {
            return a / b;
        }
        return std::pow(a, b);
    }

    GeneratedExpression Leaf() {
        if(Pick(2) == 0) {
            const ConstantSpec& c = kConstants[static_cast<std::size_t>(Pick(static_cast<int>(kConstants.size())))];
            return {std::string(c.text), std::string(c.text), c.value, kPrimary};
        }
        const Binding& b = variables_[static_cast<std::size_t>(Pick(static_cast<int>(variables_.size())))];
        return {std::string(b.name), std::string(b.name), b.value, kPrimary};
    }

    GeneratedExpression Unary(int depth) {
        const GeneratedExpression operand = Generate(depth - 1);
        const bool negate = Pick(2) == 0;
        const std::string op = negate ? "-" : "~";
        const Value value = negate ? -operand.value : detail::BitNot(operand.value);
        const std::string minimal = op + (operand.precedence < kUnary ? Wrap(operand.minimal) : operand.minimal);
        return {Wrap(op + operand.full), minimal, value, kUnary};
    }

    GeneratedExpression Binary(int depth) {
        const OperatorSpec& op = kOperators[static_cast<std::size_t>(Pick(static_cast<int>(kOperators.size())))];
        const GeneratedExpression lhs = Generate(depth - 1);
        const GeneratedExpression rhs = Generate(depth - 1);
        const Value value = Apply(op.text, lhs.value, rhs.value);

        std::string lhs_minimal;
        std::string rhs_minimal;
        if(op.precedence == kPow) {
            // Right-associative, binds tighter than unary: "(-x)^2", "(x^y)^z", but "x^-y" and "x^y^z".
            lhs_minimal = lhs.precedence <= kPow ? Wrap(lhs.minimal) : lhs.minimal;
            rhs_minimal = rhs.precedence < kUnary ? Wrap(rhs.minimal) : rhs.minimal;
        } else {
            lhs_minimal = lhs.precedence < op.precedence ? Wrap(lhs.minimal) : lhs.minimal;
            rhs_minimal = rhs.precedence <= op.precedence ? Wrap(rhs.minimal) : rhs.minimal;
        }

        const std::string spaced = " " + std::string(op.text) + " ";
        return {Wrap(lhs.full + spaced + rhs.full), lhs_minimal + spaced + rhs_minimal, value, op.precedence};
    }

    GeneratedExpression Ternary(int depth) {
        const GeneratedExpression condition = Generate(depth - 1);
        const GeneratedExpression when_true = Generate(depth - 1);
        const GeneratedExpression when_false = Generate(depth - 1);
        const Value value = condition.value != 0.0f ? when_true.value : when_false.value;
        // Only a ternary condition needs parentheses; both branches take any expression.
        const std::string condition_minimal = condition.precedence < kOr ? Wrap(condition.minimal) : condition.minimal;
        return {
            Wrap(condition.full + " ? " + when_true.full + " : " + when_false.full),
            condition_minimal + " ? " + when_true.minimal + " : " + when_false.minimal,
            value,
            kTernary
        };
    }

    GeneratedExpression Call(int depth) {
        const FunctionSpec& spec = kFunctions[static_cast<std::size_t>(Pick(static_cast<int>(kFunctions.size())))];
        const std::size_t argc = spec.max_args == kVariadic ? static_cast<std::size_t>(1 + Pick(3)) : spec.min_args;

        std::array<Value, limits::kMaxArguments> values{};
        std::string full{spec.name};
        std::string minimal{spec.name};
        full += '(';
        minimal += '(';
        for(std::size_t i = 0; i < argc; ++i) {
            const GeneratedExpression argument = Generate(depth - 1);
            values[i] = argument.value;
            full += (i == 0 ? "" : ", ") + argument.full;
            minimal += (i == 0 ? "" : ", ") + argument.minimal;
        }
        full += ')';
        minimal += ')';

        const Value value = Invoke(spec.id, std::span<const Value>{values.data(), argc});
        return {full, minimal, value, kPrimary};
    }

    std::mt19937 rng_;
    std::span<const Binding> variables_;
};

} // namespace eerie_leap::expression_engine::testing
