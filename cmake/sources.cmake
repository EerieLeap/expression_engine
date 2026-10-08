# The one list of library sources and public headers, shared by the host build
# (CMakeLists.txt) and the Zephyr module (zephyr/CMakeLists.txt). Paths are
# relative to EXPRESSION_ENGINE_ROOT, which both entry points set before
# including this file.

set(EXPRESSION_ENGINE_PUBLIC_HEADERS
    include/eerie_leap/expression_engine/compiler.hpp
    include/eerie_leap/expression_engine/disassembler.hpp
    include/eerie_leap/expression_engine/error.hpp
    include/eerie_leap/expression_engine/expression_engine.hpp
    include/eerie_leap/expression_engine/functions.hpp
    include/eerie_leap/expression_engine/instruction.hpp
    include/eerie_leap/expression_engine/limits.hpp
    include/eerie_leap/expression_engine/program.hpp
    include/eerie_leap/expression_engine/types.hpp
)

set(EXPRESSION_ENGINE_SOURCES
    src/compiler.cpp
    src/disassembler.cpp
    src/emitter.cpp
    src/error.cpp
    src/functions.cpp
    src/lexer.cpp
    src/parser.cpp
    src/program.cpp
)

list(TRANSFORM EXPRESSION_ENGINE_PUBLIC_HEADERS PREPEND "${EXPRESSION_ENGINE_ROOT}/")
list(TRANSFORM EXPRESSION_ENGINE_SOURCES PREPEND "${EXPRESSION_ENGINE_ROOT}/")

# The compile-time caps and their defaults. Each becomes an EXPRESSION_ENGINE_MAX_<CAP>
# macro; limits.hpp is the only file that reads them.
set(EXPRESSION_ENGINE_CAPS LENGTH INSTRUCTIONS VARIABLES STACK NESTING ARGUMENTS)
set(EXPRESSION_ENGINE_DEFAULT_MAX_LENGTH 256)
set(EXPRESSION_ENGINE_DEFAULT_MAX_INSTRUCTIONS 64)
set(EXPRESSION_ENGINE_DEFAULT_MAX_VARIABLES 16)
set(EXPRESSION_ENGINE_DEFAULT_MAX_STACK 16)
set(EXPRESSION_ENGINE_DEFAULT_MAX_NESTING 16)
set(EXPRESSION_ENGINE_DEFAULT_MAX_ARGUMENTS 8)
