# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(
    Zephyr
    COMPONENTS
    REQUIRED HINTS $ENV{ZEPHYR_BASE}
)

project(ocispec_test)

# ######################################################################################################################
# Definitions
# ######################################################################################################################

# Aos core configuration
add_definitions(-include aoscoreconfig.hpp)

# ######################################################################################################################
# Includes
# ######################################################################################################################

target_include_directories(app PRIVATE ${APPLICATION_SOURCE_DIR}/../../src)
target_include_directories(app PRIVATE ${APPLICATION_SOURCE_DIR}/../../../aos_core_lib_cpp/include)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

# ######################################################################################################################
# Target
# ######################################################################################################################

target_sources(app PRIVATE src/main.cpp ../../src/ocispec/ocispec.cpp)
