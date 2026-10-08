#include <array>
#include <charconv>
#include <cmath>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include <eerie_leap/expression_engine/compiler.hpp>
#include <eerie_leap/expression_engine/functions.hpp>

#include "bit_ops.hpp"
#include "support/approx.hpp"
#include "support/listing.hpp"
#include "support/printers.hpp"
#include "vectors/function_vectors.hpp"

namespace eerie_leap::expression_engine {

namespace {

// A literal that the lexer turns back into exactly `value`.
std::string Literal(Value value) {
    if(std::isnan(value)) {
        return "(0 / 0)";
    }
    if(std::isinf(value)) {
        return value > 0 ? "(1 / 0)" : "(-1 / 0)";
    }
    std::array<char, 32> digits{};
    const auto [end, ec] = std::to_chars(digits.data(), digits.data() + digits.size(), value);
    std::string text(digits.data(), static_cast<std::size_t>(end - digits.data()));
    return value < 0 ? "(" + text + ")" : text;
}

std::size_t ArgumentCount(const FunctionSpec& spec, const std::array<Value, 3>& args) {
    if(spec.max_args != kVariadic) {
        return spec.min_args;
    }
    std::size_t count = 0;
    while(count < args.size() && !std::isnan(args[count])) {
        ++count;
    }
    return count;
}

std::string Call(std::string_view name, const std::vector<std::string>& arguments) {
    std::string text{name};
    text += '(';
    for(std::size_t i = 0; i < arguments.size(); ++i) {
        text += (i == 0 ? "" : ", ") + arguments[i];
    }
    text += ')';
    return text;
}

} // namespace

TEST(Functions, TableIsSortedAndSearchable) {
    for(const FunctionSpec& spec : kFunctions) {
        EXPECT_EQ(FindFunction(spec.name), &spec) << spec.name;
        EXPECT_EQ(Spec(spec.id).name, spec.name);
        EXPECT_GE(spec.min_args, 1);
        EXPECT_TRUE(spec.max_args == kVariadic || spec.max_args >= spec.min_args) << spec.name;
    }
    EXPECT_EQ(FindFunction("rnd"), nullptr);
    EXPECT_EQ(FindFunction(""), nullptr);
    EXPECT_EQ(FindFunction("sinh2"), nullptr);
    EXPECT_EQ(FindFunction("Sin"), nullptr);
}

TEST(Functions, EveryFunctionHasAVector) {
    for(const FunctionSpec& spec : kFunctions) {
        bool found = false;
        for(const testing::FunctionVector& vector : testing::kFunctionVectors) {
            found = found || vector.name == spec.name;
        }
        EXPECT_TRUE(found) << spec.name;
    }
}

TEST(Functions, VectorsFoldedAtCompileTimeAndEvaluatedAtRunTime) {
    for(const testing::FunctionVector& vector : testing::kFunctionVectors) {
        const FunctionSpec* spec = FindFunction(vector.name);
        ASSERT_NE(spec, nullptr) << vector.name;
        const std::size_t argc = ArgumentCount(*spec, vector.args);

        std::vector<std::string> literals;
        std::vector<std::string> names;
        std::vector<Value> values;
        for(std::size_t i = 0; i < argc; ++i) {
            literals.push_back(Literal(vector.args[i]));
            names.push_back("a" + std::to_string(i));
            values.push_back(vector.args[i]);
        }

        const std::string folded_text = Call(vector.name, literals);
        const auto folded = Compile(folded_text);
        ASSERT_TRUE(folded) << folded_text << ": " << Name(folded.error().code);
        EXPECT_EQ(folded->Code().size(), 2u) << folded_text << " was not folded:\n" << testing::Listing(*folded);
        const Value folded_value = folded->Evaluate({});
        EXPECT_TRUE(testing::WithinUlps(vector.expected, folded_value, vector.ulps))
            << folded_text << " = " << folded_value << ", expected " << vector.expected;

        const std::string runtime_text = Call(vector.name, names);
        const auto runtime = Compile(runtime_text);
        ASSERT_TRUE(runtime) << runtime_text;
        ASSERT_EQ(runtime->VariableCount(), argc);
        const Value runtime_value = runtime->Evaluate(values);
        EXPECT_TRUE(testing::WithinUlps(vector.expected, runtime_value, vector.ulps))
            << runtime_text << " = " << runtime_value << ", expected " << vector.expected;
    }
}

TEST(Functions, ArgumentCountBoundaries) {
    for(const FunctionSpec& spec : kFunctions) {
        const std::size_t max = spec.max_args == kVariadic ? limits::kMaxArguments : spec.max_args;
        const auto With = [&spec](std::size_t count) { return Call(spec.name, std::vector<std::string>(count, "1")); };

        const auto too_few = Compile(With(spec.min_args - 1));
        ASSERT_FALSE(too_few) << spec.name;
        EXPECT_EQ(too_few.error().code, CompileErrorCode::WrongArgumentCount) << spec.name;

        EXPECT_TRUE(Compile(With(spec.min_args))) << spec.name;
        EXPECT_TRUE(Compile(With(max))) << spec.name;

        const auto too_many = Compile(With(max + 1));
        ASSERT_FALSE(too_many) << spec.name;
        EXPECT_EQ(too_many.error().code, CompileErrorCode::WrongArgumentCount) << spec.name;
    }
}

TEST(Functions, OneArgumentFormulasMatchTheDocumentation) {
    const Value samples[]{-2.5f, -1.0f, -0.5f, 0.0f, 0.5f, 1.0f, 2.5f, 10.0f, testing::kNaN, testing::kInf};
    for(const Value v : samples) {
        const auto Check = [v](FunctionId id, Value expected) {
            const Value actual = Invoke(id, std::span<const Value>{&v, 1});
            EXPECT_TRUE(testing::SameValue(expected, actual)) << Spec(id).name << "(" << v << ") = " << actual;
        };
        Check(FunctionId::Abs, std::fabs(v));
        Check(FunctionId::Acos, std::acos(v));
        Check(FunctionId::Acosh, std::acosh(v));
        Check(FunctionId::Asin, std::asin(v));
        Check(FunctionId::Asinh, std::asinh(v));
        Check(FunctionId::Atan, std::atan(v));
        Check(FunctionId::Atanh, std::atanh(v));
        Check(FunctionId::Cos, std::cos(v));
        Check(FunctionId::Cosh, std::cosh(v));
        Check(FunctionId::Exp, std::exp(v));
        Check(FunctionId::Ln, std::log(v));
        Check(FunctionId::Log, std::log(v));
        Check(FunctionId::Log10, std::log10(v));
        Check(FunctionId::Log2, std::log(v) / std::log(2.0f));
        Check(FunctionId::Rint, std::floor(v + 0.5f));
        Check(FunctionId::Sign, v < 0.0f ? -1.0f : (v > 0.0f ? 1.0f : 0.0f));
        Check(FunctionId::Sin, std::sin(v));
        Check(FunctionId::Sinh, std::sinh(v));
        Check(FunctionId::Sqrt, std::sqrt(v));
        Check(FunctionId::Tan, std::tan(v));
        Check(FunctionId::Tanh, std::tanh(v));
    }
}

TEST(Functions, VariadicFormulasFoldFromTheFirstArgument) {
    const std::array<Value, 3> values{3.0f, 1.0f, 2.0f};
    EXPECT_EQ(Invoke(FunctionId::Sum, values), 6.0f);
    EXPECT_EQ(Invoke(FunctionId::Avg, values), 2.0f);
    EXPECT_EQ(Invoke(FunctionId::Min, values), 1.0f);
    EXPECT_EQ(Invoke(FunctionId::Max, values), 3.0f);

    // std::min / std::max fold from the first argument, so a leading NaN wins and a later one loses.
    const std::array<Value, 2> nan_first{testing::kNaN, 1.0f};
    const std::array<Value, 2> nan_last{1.0f, testing::kNaN};
    EXPECT_TRUE(std::isnan(Invoke(FunctionId::Min, nan_first)));
    EXPECT_EQ(Invoke(FunctionId::Min, nan_last), 1.0f);
    EXPECT_TRUE(std::isnan(Invoke(FunctionId::Max, nan_first)));
    EXPECT_EQ(Invoke(FunctionId::Max, nan_last), 1.0f);

    const std::array<Value, 2> pair{1.0f, 2.0f};
    EXPECT_EQ(Invoke(FunctionId::Atan2, pair), std::atan2(1.0f, 2.0f));
    EXPECT_EQ(Invoke(FunctionId::Xor, pair), 3.0f);
}

} // namespace eerie_leap::expression_engine
