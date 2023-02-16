#
# Copyright (C) 2023 Renesas Electronics Corporation.
# Copyright (C) 2023 EPAM Systems, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)

project(aoszephyrapp)

# ######################################################################################################################
# Includes
# ######################################################################################################################

zephyr_include_directories(${CMAKE_CURRENT_BINARY_DIR})

# ######################################################################################################################
# Taget
# ######################################################################################################################

target_sources(app PRIVATE src/main.cpp)

# ######################################################################################################################
# Versioning
# ######################################################################################################################

find_package(Git)

if(NOT GIT_FOUND)
    set(GIT_EXECUTABLE git)
endif()

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMakeModules)

add_custom_target(
    version
    COMMAND
        ${CMAKE_COMMAND} -D GIT_EXECUTABLE=${GIT_EXECUTABLE} -D INPUT_FILE=${CMAKE_SOURCE_DIR}/src/version.hpp.in -D
        OUTPUT_FILE=${CMAKE_CURRENT_BINARY_DIR}/version.hpp -D GIT_SOURCE_DIR=${CMAKE_SOURCE_DIR} -P
        ${CMAKE_MODULE_PATH}/GenerateVersion.cmake
)

add_dependencies(app version)

# ######################################################################################################################
# External libs
# ######################################################################################################################

include(ExternalProject)

# ######################################################################################################################
# aos_core_lib_cpp
# ######################################################################################################################

set(AOSCORE_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/aoscore)

ExternalProject_Add(
    aoscore
    PREFIX ${AOSCORE_BUILD_DIR}
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${AOSCORE_BUILD_DIR}
    SOURCE_DIR ${APPLICATION_SOURCE_DIR}/../aos_core_lib_cpp
    BUILD_BYPRODUCTS ${AOSCORE_BUILD_DIR}/lib/libaossmcpp.a ${AOSCORE_BUILD_DIR}/lib/libaosiamcpp.a
)

# aossm

file(MAKE_DIRECTORY ${AOSCORE_BUILD_DIR}/include)

add_library(aossm STATIC IMPORTED GLOBAL)

add_dependencies(aossm aoscore)

set_target_properties(aossm PROPERTIES IMPORTED_LOCATION ${AOSCORE_BUILD_DIR}/lib/libaossmcpp.a)
set_target_properties(aossm PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${AOSCORE_BUILD_DIR}/include)

target_link_libraries(app PUBLIC aossm)
