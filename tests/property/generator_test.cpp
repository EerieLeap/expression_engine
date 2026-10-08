#include <cstdio>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <eerie_leap/expression_engine/compiler.hpp>

#include "support/approx.hpp"
#include "support/expression_generator.hpp"
#include "support/listing.hpp"
#include "support/printers.hpp"

namespace eerie_leap::expression_engine {

namespace {

bool IsLimitError(CompileErrorCode code) {
    return code == CompileErrorCode::TooLong || code == CompileErrorCode::TooManyInstructions ||
           code == CompileErrorCode::StackTooDeep || code == CompileErrorCode::NestingTooDeep;
}

} // namespace

TEST(Generator, BothRenderingsEvaluateToTheGeneratedValue) {
    const testing::Binding bindings[]{{"x", 2.5f}, {"y", -1.25f}, {"z", 7.0f}, {"w", 0.0f}};
    testing::ExpressionGenerator generator(12345, bindings);

    std::size_t total = 0;
    std::size_t skipped = 0;
    for(int i = 0; i < 3000; ++i) {
        const testing::GeneratedExpression expression = generator.Generate(3 + (i % 2));

        for(const std::string* text : {&expression.full, &expression.minimal}) {
            ++total;
            const auto program = Compile(*text);
            if(!program) {
                if(IsLimitError(program.error().code)) {
                    ++skipped;
                    continue;
                }
                FAIL() << "\"" << *text << "\" failed: " << Name(program.error().code) << " at "
                       << program.error().position;
            }

            std::vector<Value> variables(program->VariableCount());
            for(const testing::Binding& binding : bindings) {
                if(const auto index = program->VariableIndex(binding.name)) {
                    variables[*index] = binding.value;
                }
            }

            const Value actual = program->Evaluate(variables);
            EXPECT_TRUE(testing::SameValue(expression.value, actual))
                << "\"" << *text << "\" = " << actual << ", expected " << expression.value << "\n"
                << testing::Listing(*program);
        }
    }

    std::printf("generated %zu renderings, %zu skipped for exceeding a limit\n", total, skipped);
    EXPECT_LT(skipped * 10, total);
}

} // namespace eerie_leap::expression_engine
