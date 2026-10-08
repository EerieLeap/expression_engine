#include <string>
#include <string_view>

#include <gtest/gtest.h>

#include <eerie_leap/expression_engine/compiler.hpp>

#include "support/listing.hpp"
#include "support/printers.hpp"

namespace eerie_leap::expression_engine {

namespace {

std::string ListingOf(std::string_view text) {
    const auto program = Compile(text);
    if(!program) {
        ADD_FAILURE() << "\"" << text << "\" failed: " << Name(program.error().code) << " at "
                      << program.error().position;
        return {};
    }
    return testing::Listing(*program);
}

} // namespace

TEST(Parser, EmitsOperandsInEvaluationOrder) {
    EXPECT_EQ(
        ListingOf("x * 4 + 1.6"),
        "  0 PushVar     x\n"
        "  1 PushConst   4\n"
        "  2 Mul\n"
        "  3 PushConst   1.6\n"
        "  4 Add\n"
        "  5 End\n"
    );
    EXPECT_EQ(
        ListingOf("1 + 2 * x"),
        "  0 PushConst   1\n"
        "  1 PushConst   2\n"
        "  2 PushVar     x\n"
        "  3 Mul\n"
        "  4 Add\n"
        "  5 End\n"
    );
}

TEST(Parser, FoldsConstantExpressionsToOneInstruction) {
    EXPECT_EQ(ListingOf("2 * _pi"), "  0 PushConst   6.2831855\n  1 End\n");
    EXPECT_EQ(ListingOf("sum(1, 2, 3)"), "  0 PushConst   6\n  1 End\n");
    EXPECT_EQ(ListingOf("-2^2"), "  0 PushConst   -4\n  1 End\n");
    EXPECT_EQ(ListingOf("~0"), "  0 PushConst   -1\n  1 End\n");
    EXPECT_EQ(ListingOf("(1 + 2) * 3 == 9"), "  0 PushConst   1\n  1 End\n");
    EXPECT_EQ(ListingOf("0xFF00 >> 8"), "  0 PushConst   255\n  1 End\n");
}

TEST(Parser, UnaryMinusBindsLooserThanPower) {
    EXPECT_EQ(
        ListingOf("-x^2"),
        "  0 PushVar     x\n"
        "  1 PushConst   2\n"
        "  2 Pow\n"
        "  3 Neg\n"
        "  4 End\n"
    );
}

TEST(Parser, PowerIsRightAssociativeAndAllowsSignedExponent) {
    EXPECT_EQ(
        ListingOf("x^y^z"),
        "  0 PushVar     x\n"
        "  1 PushVar     y\n"
        "  2 PushVar     z\n"
        "  3 Pow\n"
        "  4 Pow\n"
        "  5 End\n"
    );
    EXPECT_EQ(
        ListingOf("x^-1"),
        "  0 PushVar     x\n"
        "  1 PushConst   -1\n"
        "  2 Pow\n"
        "  3 End\n"
    );
}

TEST(Parser, TernaryShape) {
    EXPECT_EQ(
        ListingOf("a ? b : c"),
        "  0 PushVar     a\n"
        "  1 JumpIfFalse 4\n"
        "  2 PushVar     b\n"
        "  3 Jump        5\n"
        "  4 PushVar     c\n"
        "  5 End\n"
    );
}

TEST(Parser, ShortCircuitShape) {
    EXPECT_EQ(
        ListingOf("x && y"),
        "  0 PushVar     x\n"
        "  1 AndJump     4\n"
        "  2 PushVar     y\n"
        "  3 ToBool\n"
        "  4 End\n"
    );
    EXPECT_EQ(
        ListingOf("x || y"),
        "  0 PushVar     x\n"
        "  1 OrJump      4\n"
        "  2 PushVar     y\n"
        "  3 ToBool\n"
        "  4 End\n"
    );
    // Jumps are never folded, but the constant right operand still is.
    EXPECT_EQ(
        ListingOf("1 && 0"),
        "  0 PushConst   1\n"
        "  1 AndJump     3\n"
        "  2 PushConst   0\n"
        "  3 End\n"
    );
}

TEST(Parser, BitwiseBindsTighterThanComparison) {
    EXPECT_EQ(
        ListingOf("x & 4 == 4"),
        "  0 PushVar     x\n"
        "  1 PushConst   4\n"
        "  2 BitAnd\n"
        "  3 PushConst   4\n"
        "  4 Eq\n"
        "  5 End\n"
    );
}

TEST(Parser, ConstantAtJumpTargetIsNotFoldedIntoFollowingOperator) {
    EXPECT_EQ(
        ListingOf("(0 ? 2 : 3) + 4"),
        "  0 PushConst   0\n"
        "  1 JumpIfFalse 4\n"
        "  2 PushConst   2\n"
        "  3 Jump        5\n"
        "  4 PushConst   3\n"
        "  5 PushConst   4\n"
        "  6 Add\n"
        "  7 End\n"
    );
    const auto program = Compile("(0 ? 2 : 3) + 4");
    ASSERT_TRUE(program);
    EXPECT_EQ(program->Evaluate({}), 7.0f);
}

TEST(Parser, CallsWithMixedArguments) {
    EXPECT_EQ(
        ListingOf("atan2(x, (y + 1) * 2)"),
        "  0 PushVar     x\n"
        "  1 PushVar     y\n"
        "  2 PushConst   1\n"
        "  3 Add\n"
        "  4 PushConst   2\n"
        "  5 Mul\n"
        "  6 Call        atan2/2\n"
        "  7 End\n"
    );
    EXPECT_EQ(
        ListingOf("sum(1, x, 2)"),
        "  0 PushConst   1\n"
        "  1 PushVar     x\n"
        "  2 PushConst   2\n"
        "  3 Call        sum/3\n"
        "  4 End\n"
    );
}

TEST(Parser, FunctionNamesAreOrdinaryVariablesWithoutParenthesis) {
    EXPECT_EQ(
        ListingOf("sin + 1"),
        "  0 PushVar     sin\n"
        "  1 PushConst   1\n"
        "  2 Add\n"
        "  3 End\n"
    );
}

TEST(Parser, PrecedenceTable) {
    struct Case {
        std::string_view minimal;
        std::string_view parenthesized;
    };
    const Case cases[]{
        {"1 + 2 * x", "1 + (2 * x)"},
        {"x - y - z", "(x - y) - z"},
        {"x / y * z", "(x / y) * z"},
        {"x < y == z", "(x < y) == z"},
        {"x | y & z", "x | (y & z)"},
        {"x & y << z", "x & (y << z)"},
        {"x << y + z", "x << (y + z)"},
        {"x + y & z", "(x + y) & z"},
        {"x & y == z", "(x & y) == z"},
        {"x == y && z", "(x == y) && z"},
        {"x && y || z", "(x && y) || z"},
        {"x || y && z", "x || (y && z)"},
        {"x || y ? 1 : 2", "(x || y) ? 1 : 2"},
        {"x ? y : z ? 1 : 2", "x ? y : (z ? 1 : 2)"},
        {"x ? y ? 1 : 2 : z", "x ? (y ? 1 : 2) : z"},
        {"-x * y", "(-x) * y"},
        {"~x & y", "(~x) & y"},
        {"-x >> y", "(-x) >> y"},
        {"x ^ y * z", "(x ^ y) * z"},
        {"-x ^ y", "-(x ^ y)"},
        {"x ^ -y ^ z", "x ^ (-(y ^ z))"},
        {"~x ^ y", "~(x ^ y)"},
        {"x * y ^ z", "x * (y ^ z)"},
        {"x >> y >> z", "(x >> y) >> z"},
        {"x < y < z", "(x < y) < z"},
    };
    for(const Case& c : cases) {
        EXPECT_EQ(ListingOf(c.minimal), ListingOf(c.parenthesized)) << c.minimal;
    }
}

} // namespace eerie_leap::expression_engine
