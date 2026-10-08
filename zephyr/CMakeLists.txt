# Zephyr module glue. The library sources and the cap defaults come from
# ../cmake/sources.cmake, shared with the host build in ../CMakeLists.txt.

if(CONFIG_EXPRESSION_ENGINE)
    get_filename_component(EXPRESSION_ENGINE_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
    include(${EXPRESSION_ENGINE_ROOT}/cmake/sources.cmake)

    if(NOT CONFIG_STD_CPP23)
        message(FATAL_ERROR "eerie_leap_expression_engine needs C++23: set CONFIG_STD_CPP23=y")
    endif()

    zephyr_library_named(eerie_leap_expression_engine)
    zephyr_library_sources(${EXPRESSION_ENGINE_SOURCES})
    zephyr_include_directories(${EXPRESSION_ENGINE_ROOT}/include)

    # Global, so every translation unit that includes limits.hpp sees the same caps.
    foreach(cap IN LISTS EXPRESSION_ENGINE_CAPS)
        zephyr_compile_definitions(EXPRESSION_ENGINE_MAX_${cap}=${CONFIG_EXPRESSION_ENGINE_MAX_${cap}})
    endforeach()
endif()
