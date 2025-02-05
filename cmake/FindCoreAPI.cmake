#
# Copyright (C) 2023 Renesas Electronics Corporation.
# Copyright (C) 2023 EPAM Systems, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#

# ######################################################################################################################
#  Find required packages
# ######################################################################################################################

find_program(PROTO_EXEC NAMES protoc)
if(NOT PROTO_EXEC)
    message(FATAL_ERROR "Can't find protoc.")
endif()

cmake_path(SET PROTO_INCLUDE NORMALIZE ${PROTO_EXEC}/../../include)

list(APPEND CMAKE_MODULE_PATH ${ZEPHYR_NANOPB_MODULE_DIR}/extra)

find_package(Nanopb REQUIRED)

# ######################################################################################################################
# Sources
# ######################################################################################################################

# Aos core sources
if(NOT DEFINED AOS_PROTO_SRC)
    set(AOS_PROTO_SRC proto/common/v1/common.proto proto/servicemanager/v4/servicemanager.proto
                      proto/iamanager/v5/iamanager.proto
    )
endif()

# Protobuf sources
set(SYSTEM_PROTO_SRC google/protobuf/timestamp.proto google/protobuf/empty.proto)

# ######################################################################################################################
# CORE_API_GENERATE (public function)
# CORE_API_GENERATE (CORE_API_DIR)
#   CORE_API_DIR = Variable to define dir with aos_core_api repository
# ######################################################################################################################
function(CORE_API_GENERATE CORE_API_DIR SCRIPTS_DIR)
    set(NANOPB_GENERATE_CPP_STANDALONE OFF)
    set(NANOPB_IMPORT_DIRS ${CORE_API_DIR}/proto)

    get_filename_component(CORE_API_DIR ${CORE_API_DIR} ABSOLUTE)

    if(NOT CORE_API_CXX)
        set(CORE_API_CXX g++)
    endif()

    # Generate Aos core API

    foreach(src ${AOS_PROTO_SRC})
        get_filename_component(file_name ${src} NAME_WLE)
        get_filename_component(file_path ${src} DIRECTORY)

        set(options_generator ${file_name}_options_generator)
        set(options_file ${CORE_API_DIR}/${file_path}/${file_name}.options)
        set(templ_file ${CORE_API_DIR}/${file_path}/${file_name}.templ)

        # generate C++ options generator

        add_custom_command(
            OUTPUT ${options_generator}.cpp
            COMMAND ${PYTHON_EXECUTABLE} ${SCRIPTS_DIR}/gen_pb_options.py ${templ_file} -o ${options_generator}.cpp
            DEPENDS ${CORE_API_DIR}/${src}
            VERBATIM
        )

        # build C++ options generator

        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${options_generator}
            COMMAND ${CORE_API_CXX} ${options_generator}.cpp ${CORE_API_CXX_FLAGS} -o ${options_generator}
            DEPENDS ${options_generator}.cpp
            VERBATIM
        )

        # generate options file

        add_custom_command(
            OUTPUT ${options_file}
            COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${options_generator} ${templ_file} ${options_file}
            DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${options_generator}
            VERBATIM
        )

        list(APPEND NANOPB_DEPENDS ${options_file})
    endforeach()

    list(TRANSFORM AOS_PROTO_SRC PREPEND ${CORE_API_DIR}/)

    nanopb_generate_cpp(proto_sources proto_headers RELPATH ${CORE_API_DIR} ${AOS_PROTO_SRC})
    target_sources(app PRIVATE ${proto_sources} ${proto_headers})

    # Generate system protobuf API

    list(TRANSFORM SYSTEM_PROTO_SRC PREPEND ${PROTO_INCLUDE}/)

    nanopb_generate_cpp(proto_sources proto_headers RELPATH ${PROTO_INCLUDE} ${SYSTEM_PROTO_SRC})
    target_sources(app PRIVATE ${proto_sources} ${proto_headers})

    # Copy aos protocol header

    file(COPY ${CORE_API_DIR}/aosprotocol/aosprotocol.h DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

endfunction()
