# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(dwonloader_test)

# Aos core configuration
add_definitions(-include ${APPLICATION_SOURCE_DIR}/../../src/aoscoreconfig.hpp)

# ######################################################################################################################
# Includes
# ######################################################################################################################
target_include_directories(app PRIVATE ${APPLICATION_SOURCE_DIR}/../../src)
target_include_directories(app PRIVATE ${APPLICATION_SOURCE_DIR}/../../../aos_core_lib_cpp/include)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

# ######################################################################################################################
# Target
# ######################################################################################################################
target_sources(app PRIVATE
    src/main.cpp ../../src/downloader/downloader.cpp)
