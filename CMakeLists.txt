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
# Config
# ######################################################################################################################

set(AOS_CORE_CONFIG aoscoreconfig.hpp)

# ######################################################################################################################
# Includes
# ######################################################################################################################

zephyr_include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/proto src)
zephyr_include_directories_ifdef(CONFIG_NATIVE_APPLICATION mocks/include)

# ######################################################################################################################
# Build in Aos Root CA
# ######################################################################################################################

if(CONFIG_AOS_ROOT_CA_PATH)
    target_sources(app PRIVATE src/rootca/rootca.S)
endif()

# ######################################################################################################################
# Generate OPTEE TA tables
# ######################################################################################################################

if(CONFIG_TA_DEPLOY_DIR)
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/optee)

    set(GENERATED_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/optee)

    execute_process(
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/gen_ta_src.py -s
                ${CMAKE_CURRENT_BINARY_DIR}/${CONFIG_TA_DEPLOY_DIR} -p -o ${GENERATED_DIRECTORY}/ta_table
    )

    include_directories(app PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/${CONFIG_TA_DEPLOY_DIR})
    target_sources(app PRIVATE ${GENERATED_DIRECTORY}/ta_table.c ${GENERATED_DIRECTORY}/ta_table.S)
endif()

# ######################################################################################################################
# Definitions
# ######################################################################################################################

# Aos core configuration
add_definitions(-include ${AOS_CORE_CONFIG})

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
            src/app/app.cpp
            src/clocksync/clocksync.cpp
            src/communication/channelmanager.cpp
            src/communication/tlschannel.cpp
            src/communication/channel.cpp
            src/communication/channelmanager.cpp
            src/downloader/downloader.cpp
            src/iamclient/iamclient.cpp
            src/image/imagehandler.cpp
            src/logger/logger.cpp
            src/monitoring/resourceusageprovider.cpp
            src/nodeinfoprovider/nodeinfoprovider.cpp
            src/ocispec/ocispec.cpp
            src/provisionmanager/provisionmanagercallback.cpp
            src/resourcemanager/resourcemanager.cpp
            src/runner/runner.cpp
            src/smclient/openhandler.cpp
            src/smclient/smclient.cpp
            src/storage/storage.cpp
            src/utils/checksum.cpp
            src/utils/utils.cpp
)

# use pipe for native posix
target_sources_ifdef(CONFIG_NATIVE_APPLICATION app PRIVATE src/communication/socket.cpp)

# use xenvchan for xen
target_sources_ifndef(CONFIG_NATIVE_APPLICATION app PRIVATE src/communication/xenvchan.cpp)

# Enable vchannels and xrun mocks for native posix
target_sources_ifdef(CONFIG_NATIVE_APPLICATION app PRIVATE mocks/vch/vch.cpp mocks/xrun/xrun.cpp mocks/xstat/xstat.cpp)

# Include domain IPL and dtb for xen_dom0 snippet
target_sources_ifdef(CONFIG_XEN_DOM0 app PRIVATE src/domains/dom_runner.c src/domains/domain_bins.S)

# Include source files for DomD support

target_sources_ifdef(CONFIG_SOC_SERIES_RCAR_GEN4 app PRIVATE src/domains/domd/domd_cfg_spider.c)
target_sources_ifdef(CONFIG_BOARD_RCAR_SALVATOR_XS_M3 app PRIVATE src/domains/domd/domd_cfg_salvator_xs.c)
target_sources_ifdef(CONFIG_BOARD_RCAR_H3ULCB_CA57 app PRIVATE src/domains/domd/domd_cfg_h3ulcb.c)

# Include configuration source file for DomU
target_sources_ifdef(CONFIG_DOMU_ENABLE app PRIVATE src/domains/domu/domu_cfg.c)

# Include HW-specific source file
target_sources_ifndef(CONFIG_NATIVE_APPLICATION app PRIVATE src/bsp/reboot.c src/bsp/mount.c)
target_sources_ifdef(CONFIG_FILE_SYSTEM_LITTLEFS app PRIVATE src/bsp/mount_littlefs.c)
target_sources_ifdef(CONFIG_FAT_FILESYSTEM_ELM app PRIVATE src/bsp/mount_fatfs.c)

# ######################################################################################################################
# Versioning
# ######################################################################################################################

find_package(Git)

if(NOT GIT_FOUND)
    set(GIT_EXECUTABLE git)
endif()

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

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

set(CORE_API_CXX_FLAGS
    -I${CMAKE_CURRENT_SOURCE_DIR}/src
    -I${CMAKE_CURRENT_SOURCE_DIR}/../aos_core_lib_cpp/include
    -imacros
    ${AUTOCONF_H}
    -include
    ${AOS_CORE_CONFIG}
    -DARCH_STACK_PTR_ALIGN=16
)

core_api_generate(${CMAKE_CURRENT_SOURCE_DIR}/../aos_core_api ${CMAKE_CURRENT_SOURCE_DIR}/scripts)

# ######################################################################################################################
# mbedTLS
# ######################################################################################################################

# Enable AESNI for posix 32bit
if(${CONFIG_BOARD_NATIVE_POSIX})
    target_compile_options(mbedTLS INTERFACE -mpclmul -msse2 -maes)
endif()

# WA to have get time on zephyr
target_compile_definitions_ifndef(CONFIG_NATIVE_APPLICATION mbedTLS INTERFACE _POSIX_VERSION=200809L)

# Add Aos psa driver

set(mbedtls_source_dir "${APPLICATION_SOURCE_DIR}/../modules/crypto/mbedtls")
set(aoscore_source_dir "${APPLICATION_SOURCE_DIR}/../aos_core_lib_cpp")

set_source_files_properties(
    ${mbedtls_source_dir}/library/psa_crypto_driver_wrappers_no_static.c ${mbedtls_source_dir}/library/psa_crypto.c
    TARGET_DIRECTORY mbedTLS PROPERTIES HEADER_FILE_ONLY ON
)

target_include_directories(mbedTLSCrypto PRIVATE ${mbedtls_source_dir}/library)

target_sources(
    mbedTLSCrypto
    PRIVATE ${aoscore_source_dir}/src/common/crypto/mbedtls/drivers/psa_crypto_driver_wrappers_no_static.c
            ${aoscore_source_dir}/src/common/crypto/mbedtls/drivers/psa_crypto.c ${mbedtls_source_dir}/library/pk_ecc.c
)

target_sources(mbedTLSX509 PRIVATE ${mbedtls_source_dir}/library/x509write.c)

# ######################################################################################################################
# External libs
# ######################################################################################################################

include(ExternalProject)

zephyr_get_include_directories_for_lang_as_string(CXX includes)
zephyr_get_system_include_directories_for_lang_as_string(CXX system_includes)
zephyr_get_compile_definitions_for_lang_as_string(CXX definitions)
zephyr_get_compile_options_for_lang_as_string(CXX options)

set(external_project_cxx_flags "${includes} ${system_includes} -imacros ${AUTOCONF_H} ${definitions} ${options}")

# ######################################################################################################################
# aos_core_lib_cpp
# ######################################################################################################################

# add mbedTLS includes and defines for Aos core lib
set(mbedtls_includes "-I$<JOIN:$<TARGET_PROPERTY:mbedTLS,INTERFACE_INCLUDE_DIRECTORIES>, -I>")
set(mbedtls_definitions "-D$<JOIN:$<TARGET_PROPERTY:mbedTLS,INTERFACE_COMPILE_DEFINITIONS>, -D>")

set(aos_cxx_flags
    "${external_project_cxx_flags} -include ${AOS_CORE_CONFIG} ${mbedtls_includes} '${mbedtls_definitions}' \
    -Wno-pedantic -Wno-error"
)
set(aoscore_build_dir ${CMAKE_CURRENT_BINARY_DIR}/aoscore)

ExternalProject_Add(
    aoscore
    PREFIX ${aoscore_build_dir}
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${aoscore_build_dir}
               -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
               -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
               -DCMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}
               -DCMAKE_SYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR}
               -DCMAKE_CXX_FLAGS=${aos_cxx_flags}
               -DWITH_MBEDTLS=OFF
    SOURCE_DIR ${aoscore_source_dir}
    BUILD_BYPRODUCTS ${aoscore_build_dir}/lib/libaossm.a ${aoscore_build_dir}/lib/libaosiam.a
                     ${aoscore_build_dir}/lib/libaoscommon.a ${aoscore_build_dir}/lib/libaoscrypto.a
    DEPENDS zephyr_interface
)

file(MAKE_DIRECTORY ${aoscore_build_dir}/include)

# aossm

add_library(aossm STATIC IMPORTED GLOBAL)

add_dependencies(aossm aoscore)

set_target_properties(aossm PROPERTIES IMPORTED_LOCATION ${aoscore_build_dir}/lib/libaossm.a)
set_target_properties(aossm PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${aoscore_build_dir}/include)

target_link_libraries(app PUBLIC aossm)

# aosiam

add_library(aosiam STATIC IMPORTED GLOBAL)

add_dependencies(aosiam aoscore)

set_target_properties(aosiam PROPERTIES IMPORTED_LOCATION ${aoscore_build_dir}/lib/libaosiam.a)
set_target_properties(aosiam PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${aoscore_build_dir}/include)

target_link_libraries(app PUBLIC aosiam)

# aoscommon

add_library(
    aoscommon STATIC ${aoscore_source_dir}/src/common/crypto/mbedtls/cryptoprovider.cpp
                     ${aoscore_source_dir}/src/common/crypto/mbedtls/driverwrapper.cpp
)

add_dependencies(aoscommon aoscore)

target_link_libraries(
    aoscommon PUBLIC ${aoscore_build_dir}/lib/libaoscommon.a ${aoscore_build_dir}/lib/libaoscrypto.a mbedTLSCrypto
)
target_include_directories(
    aoscommon
    PUBLIC ${aoscore_build_dir}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(app PUBLIC aoscommon)

# ######################################################################################################################
# Prepare softhsm2 config
# ######################################################################################################################

if(CONFIG_NATIVE_APPLICATION)
    set(tokendir ${CONFIG_AOS_HSM_DIR})

    if(NOT IS_ABSOLUTE ${tokendir})
        set(tokendir ${CMAKE_CURRENT_BINARY_DIR}/${tokendir})
    endif()

    file(COPY misc/softhsm2.conf DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/softhsm)
    file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/softhsm/softhsm2.conf "directories.tokendir = ${tokendir}/\n")
endif()
