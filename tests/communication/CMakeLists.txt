# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(
    Zephyr
    COMPONENTS
    REQUIRED HINTS $ENV{ZEPHYR_BASE}
)

set(CMAKE_MODULE_PATH ${APPLICATION_SOURCE_DIR}/../../CMakeModules)

project(communication_test)

# ######################################################################################################################
# Config
# ######################################################################################################################

set(AOS_CORE_CONFIG aoscoreconfig.hpp)

# ######################################################################################################################
# Definitions
# ######################################################################################################################

# Aos core configuration
add_definitions(-include ${AOS_CORE_CONFIG})

# ######################################################################################################################
#  Generate API
# ######################################################################################################################

find_package(CoreAPI)

set(CORE_API_CXX_FLAGS
    -I${APPLICATION_SOURCE_DIR}/../../src -I${APPLICATION_SOURCE_DIR}/../../../aos_core_lib_cpp/include -include
    ${CMAKE_CURRENT_BINARY_DIR}/zephyr/include/generated/autoconf.h -include ${AOS_CORE_CONFIG}
)

core_api_generate(${CMAKE_CURRENT_SOURCE_DIR}/../../../aos_core_api ${CMAKE_CURRENT_SOURCE_DIR}/../../scripts)

# ######################################################################################################################
# Includes
# ######################################################################################################################

target_include_directories(app PRIVATE ${APPLICATION_SOURCE_DIR}/../../../aos_core_lib_cpp/include)
target_include_directories(app PRIVATE ${APPLICATION_SOURCE_DIR}/../../../modules/lib/nanopb)
target_include_directories(app PRIVATE ${APPLICATION_SOURCE_DIR}/../../src)
target_include_directories(app PRIVATE ${APPLICATION_SOURCE_DIR}/src)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

# ######################################################################################################################
# Target
# ######################################################################################################################

target_sources(
    app
    PRIVATE ../../src/communication/communication.cpp
            ../../src/communication/cmclient.cpp
            ../../src/communication/iamserver.cpp
            ../../src/utils/checksum.cpp
            src/cmclient.cpp
            src/iamserver.cpp
            src/utils.cpp
)
