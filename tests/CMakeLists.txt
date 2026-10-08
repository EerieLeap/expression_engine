# GoogleTest suite. Internals (lexer, emitter) are tested through the src/ include path.

if(EXPRESSION_ENGINE_FETCH_GTEST)
    include(FetchContent)
    FetchContent_Declare(googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.18.0.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
else()
    find_package(GTest REQUIRED)
endif()

add_executable(expression_engine_tests
    support/allocation_counter.cpp
    unit/allocation_test.cpp
    unit/compiler_test.cpp
    unit/disassembler_test.cpp
    unit/emitter_test.cpp
    unit/evaluator_test.cpp
    unit/functions_test.cpp
    unit/lexer_test.cpp
    unit/parser_test.cpp
    property/generator_test.cpp
)

target_include_directories(expression_engine_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${EXPRESSION_ENGINE_ROOT}/src
)

target_link_libraries(expression_engine_tests PRIVATE
    eerie_leap::expression_engine
    GTest::gtest_main
)

set_target_properties(expression_engine_tests PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
expression_engine_apply_warnings(expression_engine_tests)

include(GoogleTest)
gtest_discover_tests(expression_engine_tests DISCOVERY_TIMEOUT 60)

# ======== Fuzzing (clang, opt-in) ========

if(EXPRESSION_ENGINE_FUZZ)
    add_executable(compile_fuzzer fuzz/compile_fuzzer.cpp)
    target_include_directories(compile_fuzzer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(compile_fuzzer PRIVATE eerie_leap::expression_engine)
    set_target_properties(compile_fuzzer PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
    target_compile_options(compile_fuzzer PRIVATE -fsanitize=fuzzer)
    target_link_options(compile_fuzzer PRIVATE -fsanitize=fuzzer)
endif()
