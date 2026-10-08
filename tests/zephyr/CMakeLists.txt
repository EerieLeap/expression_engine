cmake_minimum_required(VERSION 3.20.0)

# The repository itself is the module under test; this works both from the gauge
# workspace and when the repository is the west manifest (see ../../west.yml).
get_filename_component(EXPRESSION_ENGINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
set(EXTRA_ZEPHYR_MODULES "${EXPRESSION_ENGINE_DIR}")

if(BOARD MATCHES "^qemu_cortex_a")
    list(APPEND EXTRA_CONF_FILE "${CMAKE_CURRENT_SOURCE_DIR}/qemu_mmu.conf")
endif()

# CONFIG_MAX_XLAT_TABLES only exists on arm64.
if(BOARD MATCHES "^qemu_cortex_a53")
    list(APPEND EXTRA_CONF_FILE "${CMAKE_CURRENT_SOURCE_DIR}/qemu_arm64.conf")
endif()

# qemu_malta only declares 1 MB of SRAM; the machine actually provides far more.
if(BOARD MATCHES "^qemu_malta")
    list(APPEND EXTRA_DTC_OVERLAY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/qemu_malta_ram.overlay")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(expression_engine_zephyr_test)

target_sources(app PRIVATE src/main.cpp)
target_include_directories(app PRIVATE ${EXPRESSION_ENGINE_DIR}/tests)
