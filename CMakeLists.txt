cmake_minimum_required(VERSION 3.25)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" EXPRESSION_ENGINE_VERSION)
string(STRIP "${EXPRESSION_ENGINE_VERSION}" EXPRESSION_ENGINE_VERSION)

project(eerie_leap_expression_engine
    VERSION ${EXPRESSION_ENGINE_VERSION}
    DESCRIPTION "Compiler and allocation-free evaluator for sensor expressions"
    LANGUAGES CXX
)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(EXPRESSION_ENGINE_IS_TOP_LEVEL ON)
else()
    set(EXPRESSION_ENGINE_IS_TOP_LEVEL OFF)
endif()

option(EXPRESSION_ENGINE_BUILD_TESTS "Build the GoogleTest suite" ${EXPRESSION_ENGINE_IS_TOP_LEVEL})
option(EXPRESSION_ENGINE_BUILD_EXAMPLES "Build the examples" ${EXPRESSION_ENGINE_IS_TOP_LEVEL})
option(EXPRESSION_ENGINE_FETCH_GTEST "Fetch GoogleTest with FetchContent (OFF uses find_package)" ON)
option(EXPRESSION_ENGINE_SANITIZERS "Build everything with AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(EXPRESSION_ENGINE_WERROR "Treat warnings as errors" OFF)
option(EXPRESSION_ENGINE_FUZZ "Build the libFuzzer harness (clang only)" OFF)

set(EXPRESSION_ENGINE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
include(cmake/sources.cmake)
include(cmake/warnings.cmake)

foreach(cap IN LISTS EXPRESSION_ENGINE_CAPS)
    set(EXPRESSION_ENGINE_MAX_${cap} "${EXPRESSION_ENGINE_DEFAULT_MAX_${cap}}"
        CACHE STRING "Compile-time cap EXPRESSION_ENGINE_MAX_${cap}")
endforeach()

if(EXPRESSION_ENGINE_SANITIZERS)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

# ======== Library ========

add_library(eerie_leap_expression_engine STATIC ${EXPRESSION_ENGINE_SOURCES} ${EXPRESSION_ENGINE_PUBLIC_HEADERS})
add_library(eerie_leap::expression_engine ALIAS eerie_leap_expression_engine)

target_include_directories(eerie_leap_expression_engine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(eerie_leap_expression_engine PUBLIC cxx_std_23)
set_target_properties(eerie_leap_expression_engine PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
)

foreach(cap IN LISTS EXPRESSION_ENGINE_CAPS)
    target_compile_definitions(eerie_leap_expression_engine PUBLIC
        EXPRESSION_ENGINE_MAX_${cap}=${EXPRESSION_ENGINE_MAX_${cap}})
endforeach()

expression_engine_apply_warnings(eerie_leap_expression_engine)

# ======== Install ========

include(GNUInstallDirs)
install(TARGETS eerie_leap_expression_engine EXPORT eerie_leap_expression_engine_targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT eerie_leap_expression_engine_targets
    NAMESPACE eerie_leap::
    FILE eerie_leap_expression_engine-targets.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/eerie_leap_expression_engine)

# ======== Tests and examples ========

if(EXPRESSION_ENGINE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(EXPRESSION_ENGINE_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
