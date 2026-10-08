#include <string_view>

#include <gtest/gtest.h>

#include "emitter.hpp"
#include "support/printers.hpp"

namespace eerie_leap::expression_engine::detail {

namespace {

const std::string_view kText = "x + y + x + z";

std::string_view Slice(std::size_t offset, std::size_t length) {
    return kText.substr(offset, length);
}

} // namespace

TEST(Emitter, FoldsConstantBinaryOperation) {
    Emitter emitter(kText, Limits{});
    ASSERT_TRUE(emitter.EmitConst(2.0f));
    ASSERT_TRUE(emitter.EmitConst(3.0f));
    ASSERT_TRUE(emitter.EmitBinary(Op::Mul));
    EXPECT_TRUE(emitter.Code().empty());
    ASSERT_TRUE(emitter.EmitEnd());

    const auto code = emitter.Code();
    ASSERT_EQ(code.size(), 2u);
    EXPECT_EQ(code[0].op, Op::PushConst);
    EXPECT_EQ(code[0].imm, 6.0f);
    EXPECT_EQ(code[1].op, Op::End);
    EXPECT_EQ(emitter.MaxDepth(), 1);
}

TEST(Emitter, FoldsConstantUnaryOperation) {
    Emitter emitter(kText, Limits{});
    ASSERT_TRUE(emitter.EmitConst(2.0f));
    ASSERT_TRUE(emitter.EmitUnary(Op::Neg));
    ASSERT_TRUE(emitter.EmitUnary(Op::ToBool));
    ASSERT_TRUE(emitter.EmitEnd());
    ASSERT_EQ(emitter.Code().size(), 2u);
    EXPECT_EQ(emitter.Code()[0].imm, 1.0f);
}

TEST(Emitter, FoldsCallWithConstantArguments) {
    Emitter emitter(kText, Limits{});
    ASSERT_TRUE(emitter.EmitConst(1.0f));
    ASSERT_TRUE(emitter.EmitConst(2.0f));
    ASSERT_TRUE(emitter.EmitConst(3.0f));
    ASSERT_TRUE(emitter.EmitCall(FunctionId::Sum, 3));
    ASSERT_TRUE(emitter.EmitEnd());
    ASSERT_EQ(emitter.Code().size(), 2u);
    EXPECT_EQ(emitter.Code()[0].imm, 6.0f);
    EXPECT_EQ(emitter.MaxDepth(), 1);
}

TEST(Emitter, DoesNotFoldCallWithVariableArgument) {
    Emitter emitter(kText, Limits{});
    ASSERT_TRUE(emitter.EmitConst(1.0f));
    ASSERT_TRUE(emitter.EmitVariable(Slice(0, 1)));
    ASSERT_TRUE(emitter.EmitConst(2.0f));
    ASSERT_TRUE(emitter.EmitCall(FunctionId::Sum, 3));
    ASSERT_TRUE(emitter.EmitEnd());

    const auto code = emitter.Code();
    ASSERT_EQ(code.size(), 5u);
    EXPECT_EQ(code[0].op, Op::PushConst);
    EXPECT_EQ(code[1].op, Op::PushVar);
    EXPECT_EQ(code[2].op, Op::PushConst);
    EXPECT_EQ(code[3].op, Op::Call);
    EXPECT_EQ(code[3].aux, std::to_underlying(FunctionId::Sum));
    EXPECT_EQ(code[3].arg, 3);
    EXPECT_EQ(emitter.MaxDepth(), 3);
}

TEST(Emitter, FlushesPendingConstantsInOrderBeforeAVariable) {
    Emitter emitter(kText, Limits{});
    ASSERT_TRUE(emitter.EmitConst(1.0f));
    ASSERT_TRUE(emitter.EmitConst(2.0f));
    ASSERT_TRUE(emitter.EmitVariable(Slice(0, 1)));
    ASSERT_TRUE(emitter.EmitBinary(Op::Mul));
    ASSERT_TRUE(emitter.EmitBinary(Op::Add));
    ASSERT_TRUE(emitter.EmitEnd());

    const auto code = emitter.Code();
    ASSERT_EQ(code.size(), 6u);
    EXPECT_EQ(code[0].imm, 1.0f);
    EXPECT_EQ(code[1].imm, 2.0f);
    EXPECT_EQ(code[2].op, Op::PushVar);
    EXPECT_EQ(code[3].op, Op::Mul);
    EXPECT_EQ(code[4].op, Op::Add);
    EXPECT_EQ(emitter.MaxDepth(), 3);
}

TEST(Emitter, InternsVariableNames) {
    Emitter emitter(kText, Limits{});
    ASSERT_TRUE(emitter.EmitVariable(Slice(0, 1))); // x
    ASSERT_TRUE(emitter.EmitVariable(Slice(4, 1))); // y
    ASSERT_TRUE(emitter.EmitBinary(Op::Add));
    ASSERT_TRUE(emitter.EmitVariable(Slice(8, 1))); // x again
    ASSERT_TRUE(emitter.EmitBinary(Op::Add));
    ASSERT_TRUE(emitter.EmitVariable(Slice(12, 1))); // z
    ASSERT_TRUE(emitter.EmitBinary(Op::Add));

    ASSERT_EQ(emitter.Names().size(), 3u);
    EXPECT_EQ(emitter.Names()[0].offset, 0);
    EXPECT_EQ(emitter.Names()[1].offset, 4);
    EXPECT_EQ(emitter.Names()[2].offset, 12);
    EXPECT_EQ(emitter.Code()[0].arg, 0);
    EXPECT_EQ(emitter.Code()[1].arg, 1);
    EXPECT_EQ(emitter.Code()[3].arg, 0);
    EXPECT_EQ(emitter.Code()[5].arg, 2);
    EXPECT_EQ(emitter.MaxDepth(), 2);
}

TEST(Emitter, PatchesTernaryJumpsAndRestoresDepth) {
    Emitter emitter(kText, Limits{});
    ASSERT_TRUE(emitter.EmitVariable(Slice(0, 1)));
    const auto skip_true = emitter.EmitJump(Op::JumpIfFalse);
    ASSERT_TRUE(skip_true);
    ASSERT_TRUE(emitter.EmitConst(2.0f));
    const auto skip_false = emitter.EmitJump(Op::Jump);
    ASSERT_TRUE(skip_false);
    ASSERT_TRUE(emitter.PatchJump(*skip_true));
    ASSERT_TRUE(emitter.EmitConst(3.0f));
    ASSERT_TRUE(emitter.PatchJump(*skip_false));
    ASSERT_TRUE(emitter.EmitEnd());

    const auto code = emitter.Code();
    ASSERT_EQ(code.size(), 6u);
    EXPECT_EQ(code[1].op, Op::JumpIfFalse);
    EXPECT_EQ(code[1].arg, 4);
    EXPECT_EQ(code[2].imm, 2.0f);
    EXPECT_EQ(code[3].op, Op::Jump);
    EXPECT_EQ(code[3].arg, 5);
    EXPECT_EQ(code[4].imm, 3.0f);
    EXPECT_EQ(code[5].op, Op::End);
    EXPECT_EQ(emitter.MaxDepth(), 1);
}

TEST(Emitter, DoesNotFoldAcrossAJumpTarget) {
    // (0 ? 2 : 3) + 4: the 3 at the false label must not be folded with the 4.
    Emitter emitter(kText, Limits{});
    ASSERT_TRUE(emitter.EmitConst(0.0f));
    const auto skip_true = emitter.EmitJump(Op::JumpIfFalse);
    ASSERT_TRUE(skip_true);
    ASSERT_TRUE(emitter.EmitConst(2.0f));
    const auto skip_false = emitter.EmitJump(Op::Jump);
    ASSERT_TRUE(skip_false);
    ASSERT_TRUE(emitter.PatchJump(*skip_true));
    ASSERT_TRUE(emitter.EmitConst(3.0f));
    ASSERT_TRUE(emitter.PatchJump(*skip_false));
    ASSERT_TRUE(emitter.EmitConst(4.0f));
    ASSERT_TRUE(emitter.EmitBinary(Op::Add));
    ASSERT_TRUE(emitter.EmitEnd());

    const auto code = emitter.Code();
    ASSERT_EQ(code.size(), 8u);
    EXPECT_EQ(code[4].imm, 3.0f);
    EXPECT_EQ(code[5].imm, 4.0f);
    EXPECT_EQ(code[6].op, Op::Add);
    EXPECT_EQ(emitter.MaxDepth(), 2);
}

TEST(Emitter, AndJumpArrivesWithTheResultOnTheStack) {
    Emitter emitter(kText, Limits{});
    ASSERT_TRUE(emitter.EmitVariable(Slice(0, 1)));
    const auto jump = emitter.EmitJump(Op::AndJump);
    ASSERT_TRUE(jump);
    EXPECT_EQ(jump->arrival_depth, 1);
    ASSERT_TRUE(emitter.EmitVariable(Slice(4, 1)));
    ASSERT_TRUE(emitter.EmitUnary(Op::ToBool));
    ASSERT_TRUE(emitter.PatchJump(*jump));
    ASSERT_TRUE(emitter.EmitEnd());

    const auto code = emitter.Code();
    ASSERT_EQ(code.size(), 5u);
    EXPECT_EQ(code[1].op, Op::AndJump);
    EXPECT_EQ(code[1].arg, 4);
    EXPECT_EQ(code[3].op, Op::ToBool);
    EXPECT_EQ(emitter.MaxDepth(), 1);
}

TEST(Emitter, ReportsTooManyInstructions) {
    Emitter emitter(kText, Limits{.max_instructions = 3});
    ASSERT_TRUE(emitter.EmitVariable(Slice(0, 1)));
    ASSERT_TRUE(emitter.EmitVariable(Slice(4, 1)));
    ASSERT_TRUE(emitter.EmitBinary(Op::Add));
    const auto end = emitter.EmitEnd();
    ASSERT_FALSE(end);
    EXPECT_EQ(end.error(), CompileErrorCode::TooManyInstructions);
}

TEST(Emitter, ReportsStackTooDeepForMaterializedValuesOnly) {
    Emitter emitter(kText, Limits{.max_stack = 2});
    ASSERT_TRUE(emitter.EmitVariable(Slice(0, 1)));
    ASSERT_TRUE(emitter.EmitVariable(Slice(4, 1)));
    const auto third = emitter.EmitVariable(Slice(12, 1));
    ASSERT_FALSE(third);
    EXPECT_EQ(third.error(), CompileErrorCode::StackTooDeep);

    // Pending constants never touch the evaluation stack.
    Emitter folding(kText, Limits{.max_stack = 1});
    for(int i = 0; i < 5; ++i) {
        ASSERT_TRUE(folding.EmitConst(1.0f));
    }
    for(int i = 0; i < 4; ++i) {
        ASSERT_TRUE(folding.EmitBinary(Op::Add));
    }
    ASSERT_TRUE(folding.EmitEnd());
    EXPECT_EQ(folding.MaxDepth(), 1);
    EXPECT_EQ(folding.Code()[0].imm, 5.0f);
}

TEST(Emitter, ReportsTooManyVariables) {
    Emitter emitter(kText, Limits{.max_variables = 1});
    ASSERT_TRUE(emitter.EmitVariable(Slice(0, 1)));
    const auto second = emitter.EmitVariable(Slice(4, 1));
    ASSERT_FALSE(second);
    EXPECT_EQ(second.error(), CompileErrorCode::TooManyVariables);
    EXPECT_TRUE(emitter.EmitVariable(Slice(8, 1))); // x again is interned, not new
}

} // namespace eerie_leap::expression_engine::detail
