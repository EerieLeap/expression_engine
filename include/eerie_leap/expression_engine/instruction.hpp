#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>

#include "types.hpp"

namespace eerie_leap::expression_engine {

// Stack machine opcodes. Binary operators pop two values and push one; comparisons push 1 or 0.
// The bitwise group converts through ToInt32 (see docs/LANGUAGE.md). Jump targets are absolute
// instruction indices in Instruction::arg.
enum class Op : std::uint8_t {
    PushConst, // push imm
    PushVar,   // push variables[arg]
    Neg,
    Add,
    Sub,
    Mul,
    Div,
    Pow,
    Lt,
    Gt,
    Le,
    Ge,
    Eq,
    Ne,
    BitAnd,
    BitOr,
    BitNot,
    Shl,
    Shr,
    ToBool,      // pop v, push v != 0 ? 1 : 0
    AndJump,     // pop v; if v == 0 { push 0; jump arg }
    OrJump,      // pop v; if v != 0 { push 1; jump arg }
    JumpIfFalse, // pop v; if v == 0 jump arg
    Jump,        // jump arg
    Call,        // pop arg values, push Invoke(FunctionId{aux}, values)
    End,         // return the single remaining value
};

inline constexpr std::size_t kOpCount = 26;

struct Instruction {
    Op op;
    std::uint8_t aux;  // FunctionId for Call
    std::uint16_t arg; // variable index, jump target or argument count
    Value imm;         // PushConst
};

static_assert(sizeof(Instruction) == 8, "the instruction format is eight bytes");
static_assert(std::is_trivially_copyable_v<Instruction>);

// The enumerator's name, e.g. "PushConst".
[[nodiscard]] std::string_view Name(Op op) noexcept;

} // namespace eerie_leap::expression_engine
