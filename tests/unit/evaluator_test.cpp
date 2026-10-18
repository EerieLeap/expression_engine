#include <array>
#include <cstdint>
#include <span>
#include <string_view>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include <eerie_leap/expression_engine/compiler.hpp>

#include "bit_ops.hpp"
#include "support/approx.hpp"
#include "support/listing.hpp"
#include "support/printers.hpp"
#include "vectors/evaluation_vectors.hpp"

namespace eerie_leap::expression_engine {

namespace {

std::vector<Value> Bind(const Program& program, std::span<const testing::Binding> bindings, bool strict = true) {
    std::vector<Value> variables(program.VariableCount(), 0.0F);
    for(const testing::Binding& binding : bindings) {
        if(binding.name.empty()) {
            continue;
        }
        const auto index = program.VariableIndex(binding.name);
        if(!index) {
            if(strict) {
                ADD_FAILURE() << "unused binding " << binding.name;
            }
            continue;
        }
        variables[*index] = binding.value;
    }
    return variables;
}

Value Eval(std::string_view text, std::span<const testing::Binding> bindings = {}) {
    const auto program = Compile(text);
    if(!program) {
        ADD_FAILURE() << "\"" << text << "\" failed: " << Name(program.error().code);
        return testing::NAN_VALUE;
    }
    return program->Evaluate(Bind(*program, bindings, false));
}

} // namespace

TEST(Evaluator, EvaluationVectors) {
    for(const testing::EvaluationVector& vector : testing::EVALUATION_VECTORS) {
        const auto program = Compile(vector.expression);
        ASSERT_TRUE(program) << "\"" << vector.expression << "\" failed: " << Name(program.error().code) << " at "
                             << program.error().position;
        const Value actual = program->Evaluate(Bind(*program, vector.bindings));
        EXPECT_TRUE(testing::WithinUlps(vector.expected, actual, vector.ulps))
            << "\"" << vector.expression << "\" = " << actual << ", expected " << vector.expected << "\n"
            << testing::Listing(*program);
    }
}

TEST(Evaluator, ReturnsNaNWhenGivenTooFewVariables) {
    const auto program = Compile("x + y");
    ASSERT_TRUE(program);
    const std::array<Value, 3> values{ 1.0F, 2.0F, 3.0F };
    EXPECT_TRUE(std::isnan(program->Evaluate(std::span<const Value>{ values.data(), 1 })));
    EXPECT_EQ(program->Evaluate(std::span<const Value>{ values.data(), 2 }), 3.0F);
    EXPECT_EQ(program->Evaluate(values), 3.0F);
}

TEST(Evaluator, ToInt32EdgeCases) {
    struct Case {
        Value value;
        std::int32_t expected;
    };
    const Case cases[]{
        { testing::NAN_VALUE, 0 },
        { testing::INF_VALUE, 0 },
        { -testing::INF_VALUE, 0 },
        { 0.0F, 0 },
        { -0.0F, 0 },
        { 0.5F, 0 },
        { -0.5F, 0 },
        { 3.7F, 3 },
        { -3.7F, -3 },
        { -1.0F, -1 },
        { 16777216.0F, 16777216 },
        { 2147483648.0F, -2147483647 - 1 },
        { 4294967296.0F, 0 },
        { 4294967808.0F, 512 },
        { -4294967808.0F, -512 },
        { 9223372036854775808.0F, 0 },
        { -9223372036854775808.0F, 0 },
        { 1e30F, 0 },
    };
    for(const Case& c : cases) {
        EXPECT_EQ(detail::ToInt32(c.value), c.expected) << c.value;
    }
}

TEST(Evaluator, BitwiseOperatorsAtRunTime) {
    const testing::Binding bindings[]{ { "x", 6.0F }, { "y", 3.0F } };
    EXPECT_EQ(Eval("x & y", bindings), 2.0F);
    EXPECT_EQ(Eval("x | y", bindings), 7.0F);
    EXPECT_EQ(Eval("xor(x, y)", bindings), 5.0F);
    EXPECT_EQ(Eval("~x", bindings), -7.0F);
    EXPECT_EQ(Eval("x << y", bindings), 48.0F);
    EXPECT_EQ(Eval("x >> y", bindings), 0.0F);
    EXPECT_EQ(Eval("x >> 1", bindings), 3.0F);
}

TEST(Evaluator, ShiftCounts) {
    struct Case {
        std::string_view text;
        Value x;
        Value n;
        Value expected;
    };
    const Case cases[]{
        { "x << n", 1.0F, -1.0F, 0.0F },
        { "x << n", 1.0F, 0.0F, 1.0F },
        { "x << n", 1.0F, 2.9F, 4.0F },
        { "x << n", 1.0F, 31.0F, -2147483648.0F },
        { "x << n", 1.0F, 32.0F, 0.0F },
        { "x << n", 1.0F, 1e10F, 0.0F },
        { "x << n", 1.0F, testing::NAN_VALUE, 1.0F },
        { "x >> n", -8.0F, 1.0F, -4.0F },
        { "x >> n", -8.0F, 31.0F, -1.0F },
        { "x >> n", -8.0F, 32.0F, 0.0F },
        { "x >> n", -8.0F, -1.0F, 0.0F },
        { "x >> n", 2147483648.0F, 31.0F, -1.0F },
        { "x >> n", 1024.0F, 3.0F, 128.0F },
    };
    for(const Case& c : cases) {
        const testing::Binding bindings[]{ { "x", c.x }, { "n", c.n } };
        EXPECT_TRUE(testing::SameValue(c.expected, Eval(c.text, bindings))) << c.text << " x=" << c.x << " n=" << c.n;
    }
}

TEST(Evaluator, ShortCircuitWithNaNOperands) {
    const testing::Binding zero[]{ { "x", 0.0F } };
    const testing::Binding one[]{ { "x", 1.0F } };
    const testing::Binding nan[]{ { "x", testing::NAN_VALUE } };
    EXPECT_EQ(Eval("x && (0 / 0)", zero), 0.0F);
    EXPECT_EQ(Eval("x && (0 / 0)", one), 1.0F);
    EXPECT_EQ(Eval("x && (0 / 0)", nan), 1.0F);
    EXPECT_EQ(Eval("x || (0 / 0)", zero), 1.0F);
    EXPECT_EQ(Eval("x || 0", nan), 1.0F);
    EXPECT_EQ(Eval("x && 0", nan), 0.0F);
}

TEST(Evaluator, ComparisonsYieldExactlyOneOrZero) {
    const testing::Binding bindings[]{ { "x", 2.5F }, { "y", -1.0F } };
    for(const std::string_view text : { "x < y", "x > y", "x <= y", "x >= y", "x == y", "x != y" }) {
        const Value value = Eval(text, bindings);
        EXPECT_TRUE(value == 0.0F || value == 1.0F) << text;
    }
}

TEST(Evaluator, IsReentrantAcrossThreads) {
    const auto program = Compile("sum(x, y, 1) * atan2(x, y) + (x > y ? x : y)");
    ASSERT_TRUE(program);
    const std::array<Value, 2> variables{ 2.0F, 3.0F };
    const Value expected = program->Evaluate(variables);

    std::vector<std::thread> threads;
    std::array<bool, 4> ok{};
    threads.reserve(ok.size());
    for(bool& t : ok) {
        threads.emplace_back([&] {
            bool all = true;
            for(int i = 0; i < 1000; ++i) {
                all = all && program->Evaluate(variables) == expected;
            }
            t = all;
        });
    }
    for(std::thread& thread : threads) {
        thread.join();
    }
    for(const bool result : ok) {
        EXPECT_TRUE(result);
    }
}

} // namespace eerie_leap::expression_engine
