#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <utility>

#include "limits.hpp"
#include "types.hpp"

namespace eerie_leap::expression_engine {

// Alphabetical, so that kFunctions[std::to_underlying(id)] is the function's own entry.
enum class FunctionId : std::uint8_t {
    Abs,
    Acos,
    Acosh,
    Asin,
    Asinh,
    Atan,
    Atan2,
    Atanh,
    Avg,
    Cos,
    Cosh,
    Exp,
    Ln,
    Log,
    Log10,
    Log2,
    Max,
    Min,
    Rint,
    Sign,
    Sin,
    Sinh,
    Sqrt,
    Sum,
    Tan,
    Tanh,
    Xor,
};

// max_args of a function that takes any number of arguments up to Limits::max_arguments.
inline constexpr std::uint8_t kVariadic = 255;

struct FunctionSpec {
    std::string_view name;
    FunctionId id;
    std::uint8_t min_args;
    std::uint8_t max_args;
};

// The single source of truth: the parser looks names up here, the evaluator dispatches on the id,
// the disassembler prints the name, and the tests iterate the table.
inline constexpr std::array<FunctionSpec, 27> kFunctions{{
    {"abs", FunctionId::Abs, 1, 1},     {"acos", FunctionId::Acos, 1, 1},       {"acosh", FunctionId::Acosh, 1, 1},
    {"asin", FunctionId::Asin, 1, 1},   {"asinh", FunctionId::Asinh, 1, 1},     {"atan", FunctionId::Atan, 1, 1},
    {"atan2", FunctionId::Atan2, 2, 2}, {"atanh", FunctionId::Atanh, 1, 1},     {"avg", FunctionId::Avg, 1, kVariadic},
    {"cos", FunctionId::Cos, 1, 1},     {"cosh", FunctionId::Cosh, 1, 1},       {"exp", FunctionId::Exp, 1, 1},
    {"ln", FunctionId::Ln, 1, 1},       {"log", FunctionId::Log, 1, 1},         {"log10", FunctionId::Log10, 1, 1},
    {"log2", FunctionId::Log2, 1, 1},   {"max", FunctionId::Max, 1, kVariadic}, {"min", FunctionId::Min, 1, kVariadic},
    {"rint", FunctionId::Rint, 1, 1},   {"sign", FunctionId::Sign, 1, 1},       {"sin", FunctionId::Sin, 1, 1},
    {"sinh", FunctionId::Sinh, 1, 1},   {"sqrt", FunctionId::Sqrt, 1, 1},       {"sum", FunctionId::Sum, 1, kVariadic},
    {"tan", FunctionId::Tan, 1, 1},     {"tanh", FunctionId::Tanh, 1, 1},       {"xor", FunctionId::Xor, 2, 2},
}};

static_assert(std::ranges::is_sorted(kFunctions, {}, &FunctionSpec::name), "kFunctions is searched by name");

namespace detail {

consteval bool IdsMatchIndices() {
    for(std::size_t i = 0; i < kFunctions.size(); ++i) {
        if(std::to_underlying(kFunctions[i].id) != i) {
            return false;
        }
    }
    return true;
}

} // namespace detail

static_assert(detail::IdsMatchIndices(), "FunctionId enumerates kFunctions in order");

[[nodiscard]] constexpr const FunctionSpec& Spec(FunctionId id) noexcept {
    return kFunctions[std::to_underlying(id)];
}

// nullptr when the name is not a function.
[[nodiscard]] constexpr const FunctionSpec* FindFunction(std::string_view name) noexcept {
    const auto it = std::ranges::lower_bound(kFunctions, name, {}, &FunctionSpec::name);
    return it != kFunctions.end() && it->name == name ? &*it : nullptr;
}

// Applies the function. args.size() is within the spec's range; the evaluator and the constant
// folder both call this, so compile-time and run-time results are identical.
[[nodiscard]] Value Invoke(FunctionId id, std::span<const Value> args) noexcept;

} // namespace eerie_leap::expression_engine
