#include <memory_resource>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <eerie_leap/expression_engine/compiler.hpp>

#include "support/counting_resource.hpp"
#include "support/printers.hpp"
#include "vectors/error_vectors.hpp"

namespace eerie_leap::expression_engine {

TEST(Compiler, ReportsEveryErrorVector) {
    for(const testing::ErrorVector& vector : testing::kErrorVectors) {
        const auto program = Compile(vector.expression);
        ASSERT_FALSE(program) << "\"" << vector.expression << "\" compiled";
        EXPECT_EQ(program.error(), vector.error) << "\"" << vector.expression << "\"";
    }
}

TEST(Compiler, DescribesEveryErrorCode) {
    std::set<std::string_view> descriptions;
    std::set<std::string_view> names;
    for(std::size_t i = 0; i < kCompileErrorCodeCount; ++i) {
        const auto code = static_cast<CompileErrorCode>(i);
        EXPECT_FALSE(Describe(code).empty());
        EXPECT_FALSE(Name(code).empty());
        descriptions.insert(Describe(code));
        names.insert(Name(code));
    }
    EXPECT_EQ(descriptions.size(), kCompileErrorCodeCount);
    EXPECT_EQ(names.size(), kCompileErrorCodeCount);
    EXPECT_EQ(Describe(static_cast<CompileErrorCode>(200)), "unknown error");
}

TEST(Compiler, RejectsTextLongerThanMaxLength) {
    const std::string exact(limits::kMaxLength - 1, ' ');
    EXPECT_TRUE(Compile(exact + "1"));

    const std::string over(limits::kMaxLength, ' ');
    const auto program = Compile(over + "1");
    ASSERT_FALSE(program);
    EXPECT_EQ(program.error(), (CompileError{CompileErrorCode::TooLong, limits::kMaxLength, 1}));
}

TEST(Compiler, ClampsLimitsToTheCaps) {
    const Limits huge{
        .max_length = 65535,
        .max_instructions = 65535,
        .max_variables = 65535,
        .max_stack = 255,
        .max_nesting = 255,
        .max_arguments = 255
    };
    EXPECT_EQ(huge.Clamped(), Limits{});

    const std::string over(limits::kMaxLength, ' ');
    const auto program = Compile(over + "1", std::pmr::get_default_resource(), huge);
    ASSERT_FALSE(program);
    EXPECT_EQ(program.error().code, CompileErrorCode::TooLong);
}

TEST(Compiler, TooManyVariablesAtTheCap) {
    std::string text;
    for(std::size_t i = 0; i < limits::kMaxVariables; ++i) {
        text += (i == 0 ? "v" : "+v") + std::to_string(i);
    }
    EXPECT_TRUE(Compile(text)) << text;

    const auto position = static_cast<Position>(text.size() + 1);
    text += "+v" + std::to_string(limits::kMaxVariables);
    const auto program = Compile(text);
    ASSERT_FALSE(program);
    EXPECT_EQ(program.error(), (CompileError{CompileErrorCode::TooManyVariables, position, 3}));
}

TEST(Compiler, TooManyInstructionsAtTheCap) {
    // "x" then "+x" n times is 1 + 2n instructions plus End.
    const std::size_t fits = (limits::kMaxInstructions - 2) / 2;
    std::string text = "x";
    for(std::size_t i = 0; i < fits; ++i) {
        text += "+x";
    }
    const auto ok = Compile(text);
    ASSERT_TRUE(ok);
    EXPECT_EQ(ok->Code().size(), limits::kMaxInstructions);

    const auto position = static_cast<Position>(text.size());
    text += "+x";
    const auto program = Compile(text);
    ASSERT_FALSE(program);
    EXPECT_EQ(program.error(), (CompileError{CompileErrorCode::TooManyInstructions, position, 1}));
}

TEST(Compiler, NestingTooDeepAtTheCap) {
    const std::string open(limits::kMaxNesting, '(');
    const std::string close(limits::kMaxNesting, ')');
    EXPECT_TRUE(Compile(open + "1" + close));

    const auto program = Compile("(" + open + "1" + close + ")");
    ASSERT_FALSE(program);
    EXPECT_EQ(
        program.error(),
        (CompileError{CompileErrorCode::NestingTooDeep, static_cast<Position>(limits::kMaxNesting), 1})
    );
}

TEST(Compiler, StackTooDeepAtTheCap) {
    // x+(x+(x+(...(x+x)...))) keeps every x on the stack until the innermost addition.
    std::string text;
    for(std::size_t i = 0; i < limits::kMaxStack; ++i) {
        text += "x+(";
    }
    const auto position = static_cast<Position>(text.size());
    text += "x+x" + std::string(limits::kMaxStack, ')');
    const auto program = Compile(text);
    ASSERT_FALSE(program);
    EXPECT_EQ(program.error(), (CompileError{CompileErrorCode::StackTooDeep, position, 1}));
}

TEST(Compiler, HonoursLoweredLimits) {
    const auto resource = std::pmr::get_default_resource();

    EXPECT_TRUE(Compile("x * 4", resource, {.max_instructions = 4}));
    auto program = Compile("x * 4 + 1.6", resource, {.max_instructions = 4});
    ASSERT_FALSE(program);
    EXPECT_EQ(program.error().code, CompileErrorCode::TooManyInstructions);

    EXPECT_TRUE(Compile("x + x", resource, {.max_variables = 1}));
    program = Compile("x + y", resource, {.max_variables = 1});
    ASSERT_FALSE(program);
    EXPECT_EQ(program.error(), (CompileError{CompileErrorCode::TooManyVariables, 4, 1}));

    EXPECT_TRUE(Compile("x + y", resource, {.max_stack = 2}));
    program = Compile("x + y * z", resource, {.max_stack = 2});
    ASSERT_FALSE(program);
    EXPECT_EQ(program.error(), (CompileError{CompileErrorCode::StackTooDeep, 8, 1}));

    EXPECT_TRUE(Compile("(1)", resource, {.max_nesting = 1}));
    program = Compile("((1))", resource, {.max_nesting = 1});
    ASSERT_FALSE(program);
    EXPECT_EQ(program.error(), (CompileError{CompileErrorCode::NestingTooDeep, 1, 1}));

    EXPECT_TRUE(Compile("sum(1, 2)", resource, {.max_arguments = 2}));
    program = Compile("sum(1, 2, 3)", resource, {.max_arguments = 2});
    ASSERT_FALSE(program);
    EXPECT_EQ(program.error(), (CompileError{CompileErrorCode::WrongArgumentCount, 0, 3}));

    EXPECT_TRUE(Compile("1+2", resource, {.max_length = 3}));
    program = Compile("1+22", resource, {.max_length = 3});
    ASSERT_FALSE(program);
    EXPECT_EQ(program.error(), (CompileError{CompileErrorCode::TooLong, 3, 1}));
}

#if defined(__cpp_exceptions)
TEST(Compiler, ReportsOutOfMemoryWhenTheResourceFails) {
    testing::CountingResource failing(true);
    const auto program = Compile("x + 1", &failing);
    ASSERT_FALSE(program);
    EXPECT_EQ(program.error(), (CompileError{CompileErrorCode::OutOfMemory, 0, 0}));
}
#endif

TEST(Compiler, UsesOnlyTheGivenResource) {
    testing::CountingResource resource;
    {
        const auto program = Compile("x * 4 + 1.6", &resource);
        ASSERT_TRUE(program);
        EXPECT_EQ(program->get_allocator().resource(), &resource);
        EXPECT_GE(resource.Allocations(), 1u);
        EXPECT_LE(resource.Allocations(), 3u);
    }
    EXPECT_EQ(resource.Deallocations(), resource.Allocations());
}

TEST(Compiler, ProgramOwnsItsVariableNames) {
    std::string text = "sensor_1 + sensor_2";
    auto program = Compile(text);
    ASSERT_TRUE(program);
    text.assign(text.size(), '#');

    const Program moved = std::move(*program);
    EXPECT_EQ(moved.VariableName(0), "sensor_1");
    EXPECT_EQ(moved.VariableName(1), "sensor_2");
}

TEST(Compiler, NumbersVariablesInOrderOfFirstAppearance) {
    const auto program = Compile("b + a + b");
    ASSERT_TRUE(program);
    EXPECT_EQ(program->VariableCount(), 2u);
    EXPECT_EQ(program->VariableName(0), "b");
    EXPECT_EQ(program->VariableName(1), "a");
    EXPECT_EQ(program->VariableIndex("a"), 1u);
    EXPECT_EQ(program->VariableIndex("b"), 0u);
    EXPECT_EQ(program->VariableIndex("x"), std::nullopt);

    std::vector<std::string_view> names;
    for(const std::string_view name : program->VariableNames()) {
        names.push_back(name);
    }
    EXPECT_EQ(names, (std::vector<std::string_view>{"b", "a"}));
}

TEST(Compiler, ReportsExactMaxStackDepth) {
    const auto Depth = [](std::string_view text) {
        const auto program = Compile(text);
        return program ? program->MaxStackDepth() : 255;
    };
    EXPECT_EQ(Depth("x"), 1);
    EXPECT_EQ(Depth("2 * 3"), 1);
    EXPECT_EQ(Depth("x * 4 + 1.6"), 2);
    EXPECT_EQ(Depth("1 + 2 * x"), 3);
    EXPECT_EQ(Depth("sum(x, y, z)"), 3);
    EXPECT_EQ(Depth("x ? y : z"), 1);
    EXPECT_EQ(Depth("x && y"), 1);
}

TEST(Compiler, IgnoresSurroundingWhitespace) {
    const auto program = Compile("  1\t+\n2  ");
    ASSERT_TRUE(program);
    EXPECT_EQ(program->Evaluate({}), 3.0f);
}

} // namespace eerie_leap::expression_engine
