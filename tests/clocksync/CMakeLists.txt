# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(clocksync_test)

# ######################################################################################################################
# Config
# ######################################################################################################################

set(AOS_CORE_CONFIG aoscoreconfig.hpp)
set(aoscore_source_dir "${CMAKE_CURRENT_SOURCE_DIR}/../../../aos_core_lib_cpp")

# ######################################################################################################################
# Definitions
# ######################################################################################################################

# Aos core configuration
add_definitions(-include ${AOS_CORE_CONFIG})

# ######################################################################################################################
# Includes
# ######################################################################################################################

zephyr_include_directories(${APPLICATION_SOURCE_DIR}/..)
zephyr_include_directories(${APPLICATION_SOURCE_DIR}/../../src)
zephyr_include_directories(${APPLICATION_SOURCE_DIR}/../../../aos_core_lib_cpp/include)
zephyr_include_directories(${CMAKE_CURRENT_BINARY_DIR})

# ######################################################################################################################
# Target
# ######################################################################################################################

target_sources(
    app PRIVATE src/main.cpp ../utils/log.cpp ../../src/utils/utils.cpp ../../src/clocksync/clocksync.cpp
                ${aoscore_source_dir}/src/common/tools/time.cpp
)
