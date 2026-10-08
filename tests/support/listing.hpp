#pragma once

#include <string>
#include <string_view>

#include <eerie_leap/expression_engine/disassembler.hpp>
#include <eerie_leap/expression_engine/program.hpp>

namespace eerie_leap::expression_engine::testing {

// The disassembly as one string, one instruction per line, for snapshot comparisons.
[[nodiscard]] inline std::string Listing(const Program& program) {
    std::string listing;
    Disassemble(program, [&listing](std::string_view line) {
        listing.append(line);
        listing.push_back('\n');
    });
    return listing;
}

} // namespace eerie_leap::expression_engine::testing
