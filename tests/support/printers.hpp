#pragma once

#include <ostream>

#include <eerie_leap/expression_engine/error.hpp>

// GoogleTest prints these through PrintTo when an assertion on them fails.
namespace eerie_leap::expression_engine {

inline void PrintTo(CompileErrorCode code, std::ostream* os) {
    *os << Name(code);
}

inline void PrintTo(const CompileError& error, std::ostream* os) {
    *os << Name(error.code) << " at " << error.position << " (length " << error.length << ")";
}

} // namespace eerie_leap::expression_engine
