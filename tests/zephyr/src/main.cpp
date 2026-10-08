// Ztest suite over the shared vector tables, so the engine is exercised with the target
// toolchains, without exceptions and RTTI, on little- and big-endian machines.

#include <array>
#include <cmath>
#include <string>
#include <string_view>
#include <vector>

#include <zephyr/ztest.h>

#include <eerie_leap/expression_engine/expression_engine.hpp>

#include "support/approx.hpp"
#include "support/listing.hpp"
#include "vectors/error_vectors.hpp"
#include "vectors/evaluation_vectors.hpp"
#include "vectors/function_vectors.hpp"

namespace ee = eerie_leap::expression_engine;
namespace eet = eerie_leap::expression_engine::testing;

namespace {

std::vector<ee::Value> Bind(const ee::Program& program, std::span<const eet::Binding> bindings) {
    std::vector<ee::Value> variables(program.VariableCount(), 0.0f);
    for(const eet::Binding& binding : bindings) {
        if(binding.name.empty()) {
            continue;
        }
        const auto index = program.VariableIndex(binding.name);
        zassert_true(
            index.has_value(),
            "unused binding %.*s",
            static_cast<int>(binding.name.size()),
            binding.name.data()
        );
        variables[*index] = binding.value;
    }
    return variables;
}

std::string Literal(ee::Value value) {
    if(std::isnan(value)) {
        return "(0 / 0)";
    }
    if(std::isinf(value)) {
        return value > 0 ? "(1 / 0)" : "(-1 / 0)";
    }
    char digits[32];
    const int length = snprintf(digits, sizeof digits, "%.9g", static_cast<double>(value));
    std::string text(digits, static_cast<std::size_t>(length));
    return value < 0 ? "(" + text + ")" : text;
}

} // namespace

ZTEST_SUITE(expression_engine, NULL, NULL, NULL, NULL, NULL);

ZTEST(expression_engine, test_evaluation_vectors) {
    for(const eet::EvaluationVector& vector : eet::kEvaluationVectors) {
        const auto program = ee::Compile(vector.expression);
        zassert_true(
            program.has_value(),
            "\"%.*s\" failed to compile: %.*s",
            static_cast<int>(vector.expression.size()),
            vector.expression.data(),
            static_cast<int>(ee::Name(program.error().code).size()),
            ee::Name(program.error().code).data()
        );

        const ee::Value actual = program->Evaluate(Bind(*program, vector.bindings));
        zassert_true(
            eet::WithinUlps(vector.expected, actual, vector.ulps),
            "\"%.*s\" = %f, expected %f",
            static_cast<int>(vector.expression.size()),
            vector.expression.data(),
            static_cast<double>(actual),
            static_cast<double>(vector.expected)
        );
    }
}

ZTEST(expression_engine, test_error_vectors) {
    for(const eet::ErrorVector& vector : eet::kErrorVectors) {
        const auto program = ee::Compile(vector.expression);
        zassert_false(
            program.has_value(),
            "\"%.*s\" compiled",
            static_cast<int>(vector.expression.size()),
            vector.expression.data()
        );
        zassert_true(
            program.error() == vector.error,
            "\"%.*s\": %.*s at %u/%u, expected %.*s at %u/%u",
            static_cast<int>(vector.expression.size()),
            vector.expression.data(),
            static_cast<int>(ee::Name(program.error().code).size()),
            ee::Name(program.error().code).data(),
            program.error().position,
            program.error().length,
            static_cast<int>(ee::Name(vector.error.code).size()),
            ee::Name(vector.error.code).data(),
            vector.error.position,
            vector.error.length
        );
    }
}

ZTEST(expression_engine, test_function_vectors) {
    for(const eet::FunctionVector& vector : eet::kFunctionVectors) {
        const ee::FunctionSpec* spec = ee::FindFunction(vector.name);
        zassert_not_null(spec);

        std::size_t argc = spec->min_args;
        if(spec->max_args == ee::kVariadic) {
            argc = 0;
            while(argc < vector.args.size() && !std::isnan(vector.args[argc])) {
                ++argc;
            }
        }

        std::string folded_text{vector.name};
        std::string runtime_text{vector.name};
        folded_text += '(';
        runtime_text += '(';
        std::vector<ee::Value> values;
        for(std::size_t i = 0; i < argc; ++i) {
            folded_text += (i == 0 ? "" : ", ") + Literal(vector.args[i]);
            runtime_text += std::string(i == 0 ? "" : ", ") + "a" + std::to_string(i);
            values.push_back(vector.args[i]);
        }
        folded_text += ')';
        runtime_text += ')';

        const auto folded = ee::Compile(folded_text);
        zassert_true(folded.has_value(), "%s failed to compile", folded_text.c_str());
        zassert_equal(folded->Code().size(), 2u, "%s was not folded", folded_text.c_str());
        const ee::Value folded_value = folded->Evaluate({});
        zassert_true(
            eet::WithinUlps(vector.expected, folded_value, vector.ulps),
            "%s = %f, expected %f",
            folded_text.c_str(),
            static_cast<double>(folded_value),
            static_cast<double>(vector.expected)
        );

        const auto runtime = ee::Compile(runtime_text);
        zassert_true(runtime.has_value(), "%s failed to compile", runtime_text.c_str());
        const ee::Value runtime_value = runtime->Evaluate(values);
        zassert_true(
            eet::WithinUlps(vector.expected, runtime_value, vector.ulps),
            "%s = %f, expected %f",
            runtime_text.c_str(),
            static_cast<double>(runtime_value),
            static_cast<double>(vector.expected)
        );
    }
}

ZTEST(expression_engine, test_limits_at_the_caps) {
    const std::string open(ee::limits::kMaxNesting, '(');
    const std::string close(ee::limits::kMaxNesting, ')');
    zassert_true(ee::Compile(open + "1" + close).has_value());
    const auto too_deep = ee::Compile("(" + open + "1" + close + ")");
    zassert_false(too_deep.has_value());
    zassert_equal(too_deep.error().code, ee::CompileErrorCode::NestingTooDeep);

    std::string text;
    for(std::size_t i = 0; i < ee::limits::kMaxVariables; ++i) {
        text += (i == 0 ? "v" : "+v") + std::to_string(i);
    }
    zassert_true(ee::Compile(text).has_value());
    const auto too_many = ee::Compile(text + "+v" + std::to_string(ee::limits::kMaxVariables));
    zassert_false(too_many.has_value());
    zassert_equal(too_many.error().code, ee::CompileErrorCode::TooManyVariables);
}

ZTEST(expression_engine, test_limits_argument) {
    const auto resource = std::pmr::get_default_resource();
    zassert_true(ee::Compile("x * 4", resource, {.max_instructions = 4}).has_value());
    const auto program = ee::Compile("x * 4 + 1.6", resource, {.max_instructions = 4});
    zassert_false(program.has_value());
    zassert_equal(program.error().code, ee::CompileErrorCode::TooManyInstructions);

    const ee::Limits huge{
        .max_length = 65535,
        .max_instructions = 65535,
        .max_variables = 65535,
        .max_stack = 255,
        .max_nesting = 255,
        .max_arguments = 255
    };
    zassert_true(huge.Clamped() == ee::Limits{});
}

ZTEST(expression_engine, test_program_owns_its_names_and_is_movable) {
    std::string text = "sensor_1 + sensor_2";
    auto program = ee::Compile(text);
    zassert_true(program.has_value());
    text.assign(text.size(), '#');

    const ee::Program moved = std::move(*program);
    zassert_true(moved.VariableName(0) == "sensor_1");
    zassert_true(moved.VariableName(1) == "sensor_2");
    zassert_equal(moved.MaxStackDepth(), 2);
}

ZTEST(expression_engine, test_disassembly) {
    const auto program = ee::Compile("x * 4 + 1.6");
    zassert_true(program.has_value());
    const std::string listing = eet::Listing(*program);
    const std::string expected = "  0 PushVar     x\n"
                                 "  1 PushConst   4\n"
                                 "  2 Mul\n"
                                 "  3 PushConst   1.6\n"
                                 "  4 Add\n"
                                 "  5 End\n";
    zassert_true(listing == expected, "got:\n%s", listing.c_str());
}
