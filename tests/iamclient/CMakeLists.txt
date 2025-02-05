# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(
    Zephyr
    COMPONENTS
    REQUIRED HINTS $ENV{ZEPHYR_BASE}
)

set(CMAKE_MODULE_PATH ${APPLICATION_SOURCE_DIR}/../../cmake)

project(communication_test)

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
zephyr_include_directories(${CMAKE_CURRENT_BINARY_DIR})
zephyr_include_directories(${CMAKE_CURRENT_BINARY_DIR}/proto)

# ######################################################################################################################
#  Generate API
# ######################################################################################################################

find_package(CoreAPI)

set(CORE_API_CXX_FLAGS -I${APPLICATION_SOURCE_DIR}/../../src -I${aoscore_source_dir}/include -include
                       ${CMAKE_CURRENT_BINARY_DIR}/zephyr/include/generated/autoconf.h -include ${aoscore_config}
)

set(AOS_PROTO_SRC proto/common/v1/common.proto proto/iamanager/v5/iamanager.proto)

core_api_generate(${CMAKE_CURRENT_SOURCE_DIR}/../../../aos_core_api ${CMAKE_CURRENT_SOURCE_DIR}/../../scripts)

# ######################################################################################################################
# Target
# ######################################################################################################################

target_sources(
    app PRIVATE ../../src/communication/pbhandler.cpp ../../src/iamclient/iamclient.cpp ../../src/utils/utils.cpp
                ../utils/log.cpp ../utils/pbmessages.cpp src/main.cpp
)
