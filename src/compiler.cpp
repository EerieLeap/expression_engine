#include "eerie_leap/expression_engine/compiler.hpp"

#include <algorithm>
#include <cstddef>
#include <string_view>

#include "emitter.hpp"
#include "lexer.hpp"
#include "parser.hpp"

namespace eerie_leap::expression_engine {

std::expected<Program, CompileError> Compile(
    std::string_view text,
    std::pmr::memory_resource* resource,
    const Limits& requested
) noexcept {
    const Limits limits = requested.Clamped();

    if(text.find_first_not_of(" \t\n\r\v\f") == std::string_view::npos) {
        return std::unexpected(CompileError{CompileErrorCode::Empty, 0, 0});
    }
    if(text.size() > limits.max_length) {
        const std::size_t excess = std::min<std::size_t>(text.size() - limits.max_length, 65535);
        return std::unexpected(
            CompileError{CompileErrorCode::TooLong, limits.max_length, static_cast<std::uint16_t>(excess)}
        );
    }

    detail::Lexer lexer(text);
    detail::Emitter emitter(text, limits);
    detail::Parser parser(lexer, emitter, limits);

    if(const auto r = parser.Parse(); !r) {
        return std::unexpected(r.error());
    }
    if(const auto r = emitter.EmitEnd(); !r) {
        return std::unexpected(CompileError{r.error(), static_cast<Position>(text.size()), 0});
    }

    // The only allocations of a compilation: the code, the name table and the symbols.
    const auto build = [&]() -> Program {
        Program program{Program::allocator_type{resource}};

        const auto code = emitter.Code();
        program.code_.assign(code.begin(), code.end());

        const auto names = emitter.Names();
        std::size_t symbols_size = 0;
        for(const detail::NameRef& ref : names) {
            symbols_size += ref.length;
        }
        program.names_.reserve(names.size());
        program.symbols_.reserve(symbols_size);
        for(const detail::NameRef& ref : names) {
            program.names_.push_back(Program::NameRef{static_cast<std::uint16_t>(program.symbols_.size()), ref.length});
            program.symbols_.append(text.substr(ref.offset, ref.length));
        }

        program.max_stack_ = emitter.MaxDepth();
        return program;
    };

#if defined(__cpp_exceptions)
    try {
        return build();
    } catch(...) {
        return std::unexpected(CompileError{CompileErrorCode::OutOfMemory, 0, 0});
    }
#else
    return build();
#endif
}

} // namespace eerie_leap::expression_engine
