#include "eerie_leap/expression_engine/disassembler.hpp"

#include <array>
#include <charconv>
#include <cmath>
#include <utility>

#include "eerie_leap/expression_engine/functions.hpp"
#include "eerie_leap/expression_engine/instruction.hpp"

namespace eerie_leap::expression_engine {

namespace {

constexpr std::array<std::string_view, kOpCount> kOpNames{{
    "PushConst", "PushVar", "Neg",     "Add",    "Sub",         "Mul",    "Div",   "Pow",    "Lt",
    "Gt",        "Le",      "Ge",      "Eq",     "Ne",          "BitAnd", "BitOr", "BitNot", "Shl",
    "Shr",       "ToBool",  "AndJump", "OrJump", "JumpIfFalse", "Jump",   "Call",  "End",
}};

constexpr std::size_t kOpColumnWidth = 11; // "JumpIfFalse"

// Appends into a fixed buffer, silently truncating; the buffer is sized for the longest line.
class LineWriter {
public:
    explicit LineWriter(std::span<char> buffer) noexcept : buffer_(buffer) {}

    void Append(std::string_view text) noexcept {
        for(const char c : text) {
            if(size_ < buffer_.size()) {
                buffer_[size_++] = c;
            }
        }
    }

    void Append(std::size_t number, std::size_t width) noexcept {
        std::array<char, 24> digits{};
        const auto [end, ec] = std::to_chars(digits.data(), digits.data() + digits.size(), number);
        const std::size_t length = static_cast<std::size_t>(end - digits.data());
        for(std::size_t i = length; i < width; ++i) {
            Append(" ");
        }
        Append(std::string_view{digits.data(), length});
    }

    void Append(Value value) noexcept {
        if(std::isnan(value)) {
            Append("nan");
            return;
        }
        std::array<char, 32> digits{};
        const auto [end, ec] = std::to_chars(digits.data(), digits.data() + digits.size(), value);
        Append(std::string_view{digits.data(), static_cast<std::size_t>(end - digits.data())});
    }

    void Pad(std::size_t column) noexcept {
        while(size_ < column) {
            Append(" ");
        }
    }

    [[nodiscard]] std::string_view View() const noexcept {
        return std::string_view{buffer_.data(), size_};
    }

private:
    std::span<char> buffer_;
    std::size_t size_ = 0;
};

} // namespace

std::string_view Name(Op op) noexcept {
    const auto index = std::to_underlying(op);
    return index < kOpNames.size() ? kOpNames[index] : std::string_view{"?"};
}

namespace detail {

std::string_view FormatInstruction(const Program& program, std::size_t index, std::span<char> buffer) noexcept {
    LineWriter line(buffer);
    const Instruction in = program.Code()[index];

    line.Append(index, 3);
    line.Append(" ");
    line.Append(Name(in.op));

    switch(in.op) {
    case Op::PushConst:
        line.Pad(4 + kOpColumnWidth + 1);
        line.Append(in.imm);
        break;
    case Op::PushVar:
        line.Pad(4 + kOpColumnWidth + 1);
        line.Append(program.VariableName(in.arg));
        break;
    case Op::AndJump:
    case Op::OrJump:
    case Op::JumpIfFalse:
    case Op::Jump:
        line.Pad(4 + kOpColumnWidth + 1);
        line.Append(static_cast<std::size_t>(in.arg), 0);
        break;
    case Op::Call:
        line.Pad(4 + kOpColumnWidth + 1);
        line.Append(Spec(static_cast<FunctionId>(in.aux)).name);
        line.Append("/");
        line.Append(static_cast<std::size_t>(in.arg), 0);
        break;
    default:
        break;
    }

    return line.View();
}

} // namespace detail

} // namespace eerie_leap::expression_engine
