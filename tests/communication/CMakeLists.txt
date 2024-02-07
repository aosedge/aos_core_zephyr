# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(
    Zephyr
    COMPONENTS
    REQUIRED HINTS $ENV{ZEPHYR_BASE}
)

zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(CMAKE_MODULE_PATH ${APPLICATION_SOURCE_DIR}/../../CMakeModules)

set(aoscore_source_dir "${CMAKE_CURRENT_SOURCE_DIR}/../../../aos_core_lib_cpp")

project(communication_test)

# ######################################################################################################################
# Certificates
# ######################################################################################################################

set(CERTDIR ${CMAKE_CURRENT_BINARY_DIR}/certificates)

file(MAKE_DIRECTORY ${CERTDIR})

foreach(TYPE IN ITEMS "ca" "server" "client")
    execute_process(COMMAND openssl genrsa -out ${CERTDIR}/${TYPE}.key 2048)

    execute_process(
        COMMAND openssl req -new -key ${CERTDIR}/${TYPE}.key -out ${CERTDIR}/${TYPE}.csr -subj
                "/C=XX/ST=State/L=City/O=Organization/OU=Unit/CN=${TYPE}" COMMAND_ERROR_IS_FATAL ANY
    )
endforeach()

execute_process(
    COMMAND openssl x509 -req -days 365 -in ${CERTDIR}/ca.csr -signkey ${CERTDIR}/ca.key -out ${CERTDIR}/ca.pem
            COMMAND_ERROR_IS_FATAL ANY
)

foreach(TYPE IN ITEMS "server" "client")
    execute_process(
        COMMAND openssl x509 -req -days 365 -in ${CERTDIR}/${TYPE}.csr -CA ${CERTDIR}/ca.pem -CAkey ${CERTDIR}/ca.key
                -CAcreateserial -out ${CERTDIR}/${TYPE}.cer COMMAND_ERROR_IS_FATAL ANY
    )
endforeach()

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

set(CORE_API_CXX_FLAGS -I${APPLICATION_SOURCE_DIR}/../../src -I${aoscore_source_dir}/include -include
                       ${CMAKE_CURRENT_BINARY_DIR}/zephyr/include/generated/autoconf.h -include ${AOS_CORE_CONFIG}
)

core_api_generate(${CMAKE_CURRENT_SOURCE_DIR}/../../../aos_core_api ${CMAKE_CURRENT_SOURCE_DIR}/../../scripts)

# ######################################################################################################################
# Includes
# ######################################################################################################################

target_include_directories(app PRIVATE ${aoscore_source_dir}/include)
target_include_directories(app PRIVATE ${APPLICATION_SOURCE_DIR}/../../src)
target_include_directories(app PRIVATE ${APPLICATION_SOURCE_DIR}/src)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

# ######################################################################################################################
# Target
# ######################################################################################################################

# Enable AESNI for posix 32bit
target_compile_options(mbedTLS INTERFACE -mpclmul -msse2 -maes)

target_compile_definitions(app PRIVATE CERT_DIR="${CERTDIR}")
target_compile_options(app PRIVATE -DCONFIG_AOS_ROOT_CA_PATH="${CERTDIR}/ca.pem")
target_sources(
    app
    PRIVATE ../../src/communication/cmclient.cpp
            ../../src/communication/communication.cpp
            ../../src/communication/iamserver.cpp
            ../../src/communication/tlschannel.cpp
            ../../src/rootca/rootca.S
            ../../src/utils/checksum.cpp
            src/cmclient.cpp
            src/iamserver.cpp
            src/tlschannel.cpp
            src/utils.cpp
            ${aoscore_source_dir}/src/common/crypto/mbedtls/driverwrapper.cpp
)

set_source_files_properties(
    ${ZEPHYR_MBEDTLS_MODULE_DIR}/library/psa_crypto_driver_wrappers_no_static.c
    ${ZEPHYR_MBEDTLS_MODULE_DIR}/library/psa_crypto.c TARGET_DIRECTORY mbedTLS PROPERTIES HEADER_FILE_ONLY ON
)

target_sources(
    app PRIVATE ${aoscore_source_dir}/src/common/crypto/mbedtls/drivers/psa_crypto_driver_wrappers_no_static.c
                ${aoscore_source_dir}/src/common/crypto/mbedtls/drivers/psa_crypto.c
)
