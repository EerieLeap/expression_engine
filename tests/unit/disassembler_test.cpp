#include <set>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

#include <eerie_leap/expression_engine/compiler.hpp>
#include <eerie_leap/expression_engine/disassembler.hpp>
#include <eerie_leap/expression_engine/instruction.hpp>

#include "support/listing.hpp"
#include "support/printers.hpp"

namespace eerie_leap::expression_engine {

TEST(Disassembler, FormatsEveryOperandKind) {
    const auto program = Compile("x ? sum(1, y, 2) : 3");
    ASSERT_TRUE(program);
    EXPECT_EQ(
        testing::Listing(*program),
        "  0 PushVar     x\n"
        "  1 JumpIfFalse 7\n"
        "  2 PushConst   1\n"
        "  3 PushVar     y\n"
        "  4 PushConst   2\n"
        "  5 Call        sum/3\n"
        "  6 Jump        8\n"
        "  7 PushConst   3\n"
        "  8 End\n"
    );
}

TEST(Disassembler, FormatsSpecialValues) {
    const auto Line = [](std::string_view text) {
        const auto program = Compile(text);
        if(!program) {
            ADD_FAILURE() << text;
            return std::string{};
        }
        std::string first;
        Disassemble(*program, [&first](std::string_view line) {
            if(first.empty()) {
                first = line;
            }
        });
        return first;
    };
    EXPECT_EQ(Line("1 / 0"), "  0 PushConst   inf");
    EXPECT_EQ(Line("-1 / 0"), "  0 PushConst   -inf");
    EXPECT_EQ(Line("0 / 0"), "  0 PushConst   nan");
    EXPECT_EQ(Line("1e10"), "  0 PushConst   1e+10");
    EXPECT_EQ(Line("0.1"), "  0 PushConst   0.1");
    EXPECT_EQ(Line("0xFFFFFFFF"), "  0 PushConst   -1");
}

TEST(Disassembler, NamesEveryOpcode) {
    std::set<std::string_view> names;
    for(std::size_t i = 0; i < kOpCount; ++i) {
        const std::string_view name = Name(static_cast<Op>(i));
        EXPECT_FALSE(name.empty()) << i;
        EXPECT_NE(name, "?") << i;
        names.insert(name);
    }
    EXPECT_EQ(names.size(), kOpCount);
    EXPECT_EQ(Name(static_cast<Op>(255)), "?");
    EXPECT_EQ(Name(Op::End), "End");
    EXPECT_EQ(Name(Op::JumpIfFalse), "JumpIfFalse");
}

TEST(Disassembler, CallsSinkOncePerInstruction) {
    const auto program = Compile("x * 4 + 1.6");
    ASSERT_TRUE(program);
    std::size_t lines = 0;
    Disassemble(*program, [&lines](std::string_view) { ++lines; });
    EXPECT_EQ(lines, program->Code().size());
}

} // namespace eerie_leap::expression_engine
