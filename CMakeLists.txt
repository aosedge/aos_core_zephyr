#
# Copyright (C) 2023 Renesas Electronics Corporation.
# Copyright (C) 2023 EPAM Systems, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)

project(aoszephyrapp)

# ######################################################################################################################
# Includes
# ######################################################################################################################

zephyr_include_directories(${CMAKE_CURRENT_BINARY_DIR} src)

if(CONFIG_BOARD MATCHES "rcar")
    zephyr_include_directories(${CMAKE_CURRENT_BINARY_DIR} prebuilt)
endif()

if(BOARD MATCHES "native_posix*")
    zephyr_include_directories(${CMAKE_CURRENT_BINARY_DIR} mocks/include)
endif()

# ######################################################################################################################
# Definitions
# ######################################################################################################################

# Aos core configuration
add_definitions(-include aoscoreconfig.hpp)

# ######################################################################################################################
# Compile options
# ######################################################################################################################

# Add stack usage warning
target_compile_options(app PRIVATE -Wstack-usage=2048)

# ######################################################################################################################
# Taget
# ######################################################################################################################

target_sources(
    app
    PRIVATE src/main.cpp
            src/logger/logger.cpp
            src/app/app.cpp
            src/cmclient/cmclient.cpp
            src/runner/runner.cpp
            src/storage/storage.cpp
            src/downloader/downloader.cpp
            src/resourcemanager/resourcemanager.cpp
            src/ocispec/ocispec.cpp
)

# Enable vchannels and xrun mocks for native posix
target_sources_ifdef(CONFIG_NATIVE_APPLICATION app PRIVATE mocks/vch/vch.cpp mocks/xrun/xrun.cpp)

# NOTE: Include source files with DomD binaries and configuration, needed to start DomD on Zephyr
target_sources_ifdef(
    CONFIG_SOC_SERIES_RCAR_GEN4 app PRIVATE src/domains/domd/domd_cfg_spider.c src/domains/domd/domain_bins_spider.S
)

# NOTE: Include source files for DomD support for Gen3 board.
# Gen3 specific binaries were added with separate .S file to avoid extra checks inside source files.
target_sources_ifdef(
    CONFIG_BOARD_RCAR_SALVATOR_XS_M3_2X4G app PRIVATE src/domains/domd/domd_cfg_gen3.c
    src/domains/domd/domain_bins_gen3.S
)

# NOTE: Include source files for xenvm boards.
target_sources_ifdef(CONFIG_BOARD_XENVM app PRIVATE src/domains/domd/domd_cfg_xenvm.c)

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
#  Generate API
# ######################################################################################################################

find_package(CoreAPI)

generate_api(${CMAKE_CURRENT_SOURCE_DIR}/../aos_core_api)

# ######################################################################################################################
# External libs
# ######################################################################################################################

include(ExternalProject)

zephyr_get_include_directories_for_lang_as_string(CXX includes)
zephyr_get_system_include_directories_for_lang_as_string(CXX system_includes)
zephyr_get_compile_definitions_for_lang_as_string(CXX definitions)
zephyr_get_compile_options_for_lang_as_string(CXX options)

set(external_project_cxx_flags "${includes} ${system_includes} ${definitions} ${options}")

# ######################################################################################################################
# Xen on QEMU
# ######################################################################################################################

if(CONFIG_BOARD MATCHES "xenvm")
    set(xen_build_dir ${CMAKE_CURRENT_BINARY_DIR}/xen)

    ExternalProject_Add(
        hypervisor
        GIT_REPOSITORY https://github.com/xen-project/xen
        GIT_TAG RELEASE-4.17.0
        SOURCE_DIR ${xen_build_dir}
        BUILD_IN_SOURCE 1
        CONFIGURE_COMMAND ./configure --disable-docs --disable-tools --disable-stubdom
        BUILD_COMMAND ${MAKE} make -C ${xen_build_dir} CONFIG_QEMU_XEN=y CONFIG_DEBUG=y XSM_ENABLE=y
                      XEN_TARGET_ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} HOSTCC=gcc
        INSTALL_COMMAND cp ${xen_build_dir}/xen/xen ${ZEPHYR_BINARY_DIR}
        BUILD_BYPRODUCTS ${ZEPHYR_BINARY_DIR}/xen
        DEPENDS app
    )

    add_dependencies(hypervisor zephyr_final)

    set(utils_scripts_dir ${CMAKE_CURRENT_SOURCE_DIR}/scripts)

    if(DTC)
        add_custom_command(
            TARGET hypervisor
            POST_BUILD
            COMMAND ${utils_scripts_dir}/fix_dtb.sh ${utils_scripts_dir}/gicv2.tmpl ${ZEPHYR_BINARY_DIR}/virt_gicv2.dtb
                    ${ZEPHYR_BINARY_DIR}/zephyr.bin
        )
    else()
        message(WARNING "No dtc program")
    endif()
endif()

# ######################################################################################################################
# aos_core_lib_cpp
# ######################################################################################################################

set(aos_cxx_flags "${external_project_cxx_flags} -Wno-pedantic -Wno-error -include aoscoreconfig.hpp")
set(aoscore_build_dir ${CMAKE_CURRENT_BINARY_DIR}/aoscore)

ExternalProject_Add(
    aoscore
    PREFIX ${aoscore_build_dir}
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${aoscore_build_dir} -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
               -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER} -DCMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}
               -DCMAKE_SYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR} -DCMAKE_CXX_FLAGS=${aos_cxx_flags}
    SOURCE_DIR ${APPLICATION_SOURCE_DIR}/../aos_core_lib_cpp
    BUILD_BYPRODUCTS ${aoscore_build_dir}/lib/libaossmcpp.a ${aoscore_build_dir}/lib/libaosiamcpp.a
    DEPENDS zephyr_interface
)

# aossm

file(MAKE_DIRECTORY ${aoscore_build_dir}/include)

add_library(aossm STATIC IMPORTED GLOBAL)

add_dependencies(aossm aoscore)

set_target_properties(aossm PROPERTIES IMPORTED_LOCATION ${aoscore_build_dir}/lib/libaossmcpp.a)
set_target_properties(aossm PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${aoscore_build_dir}/include)

target_link_libraries(app PUBLIC aossm)
