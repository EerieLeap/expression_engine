#include <array>
#include <cmath>
#include <string_view>

#include <gtest/gtest.h>

#include <eerie_leap/expression_engine/compiler.hpp>
#include <eerie_leap/expression_engine/disassembler.hpp>

#include "support/allocation_counter.hpp"
#include "support/counting_resource.hpp"
#include "support/printers.hpp"

namespace eerie_leap::expression_engine {

TEST(Allocation, CompileAllocatesOnceForCodeOnceForNamesAndOnceForLongSymbols) {
    struct Case {
        std::string_view text;
        std::size_t allocations;
    };
    const Case cases[]{
        {"2 * 3", 1},               // code
        {"x", 2},                   // code + names; "x" fits the small-string buffer
        {"x * 4 + 1.6", 2},         //
        {"sensor_1 + 8.34", 2},     // 8 characters still fit
        {"sensor_1 + sensor_2", 3}, // 16 characters do not
        {"sensor_1 + sensor_2 * sensor_3", 3},
    };
    for(const Case& c : cases) {
        testing::CountingResource resource;
        const auto program = Compile(c.text, &resource);
        ASSERT_TRUE(program) << c.text;
        EXPECT_EQ(resource.Allocations(), c.allocations) << c.text;
    }
}

TEST(Allocation, CompileAllocatesOnlyThroughTheResource) {
    testing::CountingResource resource; // forwards to new_delete_resource, which the counter sees
    const std::size_t before = testing::GlobalAllocations();
    const auto program = Compile("sensor_1 + sensor_2 * atan2(x, 2) + (x > 1 ? x : -x)", &resource);
    const std::size_t after = testing::GlobalAllocations();
    ASSERT_TRUE(program);
    EXPECT_EQ(after - before, resource.Allocations());
}

TEST(Allocation, FailedCompilationDoesNotAllocate) {
    testing::CountingResource resource;
    const std::size_t before = testing::GlobalAllocations();
    const auto program = Compile("sensor_1 + * 2", &resource);
    EXPECT_FALSE(program);
    EXPECT_EQ(testing::GlobalAllocations(), before);
    EXPECT_EQ(resource.Allocations(), 0u);
}

TEST(Allocation, EvaluateDoesNotAllocate) {
    const auto program = Compile("sum(x, y, z) * atan2(x, y) + (x > y ? x & 0xFF : ~y) + sqrt(z)");
    ASSERT_TRUE(program);
    const std::array<Value, 3> variables{2.0f, 3.0f, 4.0f};
    (void)program->Evaluate(variables);

    const std::size_t before = testing::GlobalAllocations();
    Value sink = 0.0f;
    for(int i = 0; i < 1000; ++i) {
        sink += program->Evaluate(variables);
    }
    EXPECT_EQ(testing::GlobalAllocations(), before);
    EXPECT_FALSE(std::isnan(sink));
}

TEST(Allocation, DisassembleDoesNotAllocate) {
    const auto program = Compile("x ? sum(1, y, 2) : 3.25");
    ASSERT_TRUE(program);
    const std::size_t before = testing::GlobalAllocations();
    std::size_t characters = 0;
    Disassemble(*program, [&characters](std::string_view line) { characters += line.size(); });
    EXPECT_EQ(testing::GlobalAllocations(), before);
    EXPECT_GT(characters, 0u);
}

} // namespace eerie_leap::expression_engine
