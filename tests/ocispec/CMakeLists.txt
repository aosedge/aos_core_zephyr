# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(
    Zephyr
    COMPONENTS
    REQUIRED HINTS $ENV{ZEPHYR_BASE}
)

project(ocispec_test)

# ######################################################################################################################
# Config
# ######################################################################################################################

set(aoscore_config aoscoreconfig.hpp)
set(aoscore_source_dir "${CMAKE_CURRENT_SOURCE_DIR}/../../../aos_core_lib_cpp")

# ######################################################################################################################
# Definitions
# ######################################################################################################################

# Aos core configuration
add_definitions(-include ${aoscore_config})

# ######################################################################################################################
# Includes
# ######################################################################################################################

zephyr_include_directories(${aoscore_source_dir}/include)
zephyr_include_directories(${APPLICATION_SOURCE_DIR}/..)
zephyr_include_directories(${APPLICATION_SOURCE_DIR}/../../src)
zephyr_include_directories(${APPLICATION_SOURCE_DIR}/src)

# ######################################################################################################################
# Target
# ######################################################################################################################

target_sources(
    app PRIVATE src/main.cpp ../../src/ocispec/ocispec.cpp ../utils/log.cpp
                ${aoscore_source_dir}/src/common/tools/fs.cpp
)
