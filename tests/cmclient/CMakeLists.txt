# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(
    Zephyr
    COMPONENTS unittest
    REQUIRED HINTS $ENV{ZEPHYR_BASE}
)

project(cmclient_test)

# ######################################################################################################################
# Includes
# ######################################################################################################################

target_include_directories(testbinary PRIVATE ${APPLICATION_SOURCE_DIR}/../../src)
target_include_directories(testbinary PRIVATE ${APPLICATION_SOURCE_DIR}/../../../aos_core_lib_cpp/include)

# ######################################################################################################################
# Taget
# ######################################################################################################################

target_sources(testbinary PRIVATE src/main.cpp ../../src/cmclient/cmclient.cpp)
