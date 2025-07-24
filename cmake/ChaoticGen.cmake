# Implementation note: public functions here should be usable even without a direct include of this script, so the
# functions should not rely on non-cache variables being present.

include_guard(GLOBAL)

function(_userver_prepare_chaotic)
    include("${CMAKE_CURRENT_LIST_DIR}/UserverTestsuite.cmake")

    find_program(
        CHAOTIC_BIN chaotic-gen
        PATHS "${CMAKE_CURRENT_SOURCE_DIR}/../chaotic/bin" "${CMAKE_CURRENT_LIST_DIR}/../../../bin"
        NO_DEFAULT_PATH
    )
    message(STATUS "Found chaotic-gen: ${CHAOTIC_BIN}")
    set_property(GLOBAL PROPERTY userver_chaotic_bin "${CHAOTIC_BIN}")

    find_program(
        CHAOTIC_DYNAMIC_CONFIGS_BIN chaotic-gen-dynamic-configs
        PATHS "${CMAKE_CURRENT_SOURCE_DIR}/../chaotic/bin-dynamic-configs" "${CMAKE_CURRENT_LIST_DIR}/../../../bin"
        NO_DEFAULT_PATH
    )
    message(STATUS "Found chaotic-gen-dynamic-configs: ${CHAOTIC_DYNAMIC_CONFIGS_BIN}")
    set_property(GLOBAL PROPERTY userver_chaotic_dynamic_configs_bin "${CHAOTIC_DYNAMIC_CONFIGS_BIN}")

    find_program(
        CHAOTIC_OPENAPI_BIN chaotic-openapi-gen
        PATHS "${CMAKE_CURRENT_SOURCE_DIR}/../chaotic-openapi/bin" "${CMAKE_CURRENT_LIST_DIR}/../../../bin"
        NO_DEFAULT_PATH
    )
    message(STATUS "Found chaotic-openapi-gen: ${CHAOTIC_OPENAPI_BIN}")
    set_property(GLOBAL PROPERTY userver_chaotic_openapi_bin "${CHAOTIC_OPENAPI_BIN}")

    find_program(CLANG_FORMAT_BIN clang-format)
    message(STATUS "Found clang-format: ${CLANG_FORMAT_BIN}")
    set_property(GLOBAL PROPERTY userver_clang_format_bin "${CLANG_FORMAT_BIN}")
    option(USERVER_CHAOTIC_FORMAT "Whether to format generated code" ON)

    if(NOT USERVER_CHAOTIC_SCRIPTS_PATH)
        get_filename_component(USERVER_DIR "${CMAKE_CURRENT_LIST_DIR}" DIRECTORY)
        set(USERVER_CHAOTIC_SCRIPTS_PATH "${USERVER_DIR}/scripts/chaotic")
    endif()

    userver_venv_setup(
        NAME userver-chaotic
        PYTHON_OUTPUT_VAR USERVER_CHAOTIC_PYTHON_BINARY
        REQUIREMENTS "${USERVER_CHAOTIC_SCRIPTS_PATH}/requirements.txt"
        UNIQUE
    )
    set_property(GLOBAL PROPERTY userver_chaotic_python_binary "${USERVER_CHAOTIC_PYTHON_BINARY}")
endfunction()

_userver_prepare_chaotic()

# Generates ${TARGET} cmake target for C++ types, parsers, serializers from JSONSchema file(s).
#
# Options: - OUTPUT_DIR - where to put generated .cpp/.hpp/.ipp files, usually ${CMAKE_CURRENT_BINARY_DIR}/smth -
# RELATIVE_TO - --relative-to option to chaotic-gen - FORMAT - can be ON/OFF, enable to format generated files, defaults
# to USERVER_CHAOTIC_FORMAT - SCHEMAS - JSONSchema source files - ARGS - extra args to chaotic-gen -
# INSTALL_INCLUDES_COMPONENT - component to install generated includes
function(userver_target_generate_chaotic TARGET)
    set(OPTIONS GENERATE_SERIALIZERS PARSE_EXTRA_FORMATS)
    set(ONE_VALUE_ARGS OUTPUT_DIR RELATIVE_TO FORMAT INSTALL_INCLUDES_COMPONENT OUTPUT_PREFIX ERASE_PATH_PREFIX)
    set(MULTI_VALUE_ARGS SCHEMAS LAYOUT INCLUDE_DIRS)
    cmake_parse_arguments(PARSE "${OPTIONS}" "${ONE_VALUE_ARGS}" "${MULTI_VALUE_ARGS}" ${ARGN})

    get_property(CHAOTIC_BIN GLOBAL PROPERTY userver_chaotic_bin)
    get_property(CHAOTIC_EXTRA_ARGS GLOBAL PROPERTY userver_chaotic_extra_args)
    get_property(USERVER_CHAOTIC_PYTHON_BINARY GLOBAL PROPERTY userver_chaotic_python_binary)
    get_property(CLANG_FORMAT_BIN GLOBAL PROPERTY userver_clang_format_bin)

    if(NOT DEFINED PARSE_OUTPUT_DIR)
        set(PARSE_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}")
    endif()
    file(MAKE_DIRECTORY "${PARSE_OUTPUT_DIR}")

    if(NOT PARSE_SCHEMAS)
        message(FATAL_ERROR "SCHEMAS is required")
    endif()

    if(NOT PARSE_RELATIVE_TO)
        message(FATAL_ERROR "RELATIVE_TO is required")
    endif()

    if(PARSE_FORMAT OR (NOT DEFINED PARSE_FORMAT AND USERVER_CHAOTIC_FORMAT))
        if(NOT CLANG_FORMAT_BIN)
            message(FATAL_ERROR "clang-format is not found and FORMAT option is set. Please install clang-format.")
        endif()
        set(CLANG_FORMAT "${CLANG_FORMAT_BIN}")
    else()
        set(CLANG_FORMAT "")
    endif()

    set(SCHEMAS)
    foreach(PARSE_SCHEMA ${PARSE_SCHEMAS})
        file(RELATIVE_PATH SCHEMA "${PARSE_RELATIVE_TO}" "${PARSE_SCHEMA}")

        string(REGEX REPLACE "^(.*)\\.([^.]*)\$" "\\1" SCHEMA "${SCHEMA}")

        list(
            APPEND
            SCHEMAS
            "${PARSE_OUTPUT_DIR}/${PARSE_OUTPUT_PREFIX}/${SCHEMA}.cpp"
            "${PARSE_OUTPUT_DIR}/${PARSE_OUTPUT_PREFIX}/${SCHEMA}.hpp"
            "${PARSE_OUTPUT_DIR}/${PARSE_OUTPUT_PREFIX}/${SCHEMA}_fwd.hpp"
            "${PARSE_OUTPUT_DIR}/${PARSE_OUTPUT_PREFIX}/${SCHEMA}_parsers.ipp"
        )
    endforeach()

    set(CHAOTIC_ARGS)

    foreach(MAP_ITEM ${PARSE_LAYOUT})
        list(APPEND CHAOTIC_ARGS "-n" "${MAP_ITEM}")
    endforeach()

    foreach(INCLUDE_DIR ${PARSE_INCLUDE_DIRS})
        list(APPEND CHAOTIC_ARGS "-I" "${INCLUDE_DIR}")
    endforeach()

    if(PARSE_GENERATE_SERIALIZERS)
        list(APPEND CHAOTIC_ARGS "--generate-serializers")
    endif()

    if(PARSE_PARSE_EXTRA_FORMATS)
        list(APPEND CHAOTIC_ARGS "--parse-extra-formats")
    endif()

    if(PARSE_ERASE_PATH_PREFIX)
        list(APPEND CHAOTIC_ARGS "-e" "${PARSE_ERASE_PATH_PREFIX}")
    endif()

    list(APPEND CHAOTIC_ARGS "-o" "${PARSE_OUTPUT_DIR}/${PARSE_OUTPUT_PREFIX}")
    list(APPEND CHAOTIC_ARGS "--relative-to" "${PARSE_RELATIVE_TO}")
    list(APPEND CHAOTIC_ARGS "--clang-format" "${CLANG_FORMAT}")

    _userver_initialize_codegen_flag()
    add_custom_command(
        OUTPUT ${SCHEMAS}
        COMMAND ${CMAKE_COMMAND} -E env "USERVER_PYTHON=${USERVER_CHAOTIC_PYTHON_BINARY}" "${CHAOTIC_BIN}"
                ${CHAOTIC_EXTRA_ARGS} ${CHAOTIC_ARGS} ${PARSE_SCHEMAS}
        DEPENDS ${PARSE_SCHEMAS}
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        VERBATIM ${CODEGEN}
    )
    _userver_codegen_register_files("${SCHEMAS}")
    add_library("${TARGET}" ${SCHEMAS})
    target_link_libraries("${TARGET}" userver::chaotic)
    target_include_directories("${TARGET}" PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>")
    target_include_directories("${TARGET}" PUBLIC "$<BUILD_INTERFACE:${PARSE_OUTPUT_DIR}>")

    if(PARSE_INSTALL_INCLUDES_COMPONENT)
        foreach(FILE ${SCHEMAS})
            string(REGEX REPLACE "^(.*)\\.([^.]*)\$" "\\2" SUFFIX "${FILE}")
            if(SUFFIX STREQUAL cpp)
                continue()
            endif()

            _userver_directory_install(
                COMPONENT ${PARSE_INSTALL_INCLUDES_COMPONENT}
                FILES "${FILE}"
                DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${PARSE_OUTPUT_PREFIX}"
            )
        endforeach()
    endif()
endfunction()

function(userver_target_generate_openapi_client TARGET)
    set(OPTIONS)
    set(ONE_VALUE_ARGS OUTPUT_DIR NAME)
    set(MULTI_VALUE_ARGS SCHEMAS ARGS)
    cmake_parse_arguments(PARSE "${OPTIONS}" "${ONE_VALUE_ARGS}" "${MULTI_VALUE_ARGS}" ${ARGN})

    get_property(CHAOTIC_OPENAPI_BIN GLOBAL PROPERTY userver_chaotic_openapi_bin)
    get_property(CHAOTIC_EXTRA_ARGS GLOBAL PROPERTY userver_chaotic_extra_args)
    get_property(USERVER_CHAOTIC_PYTHON_BINARY GLOBAL PROPERTY userver_chaotic_python_binary)

    if(NOT DEFINED PARSE_OUTPUT_DIR)
        set(PARSE_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/${PARSE_NAME}")
    endif()
    file(MAKE_DIRECTORY "${PARSE_OUTPUT_DIR}")

    if(NOT PARSE_SCHEMAS)
        message(FATAL_ERROR "SCHEMAS is required")
    endif()

    if(NOT PARSE_NAME)
        message(FATAL_ERROR "NAME is required")
    endif()

    set(SCHEMAS
        "${PARSE_OUTPUT_DIR}/include/client/${PARSE_NAME}/client.hpp"
        "${PARSE_OUTPUT_DIR}/include/client/${PARSE_NAME}/client_impl.hpp"
        "${PARSE_OUTPUT_DIR}/include/client/${PARSE_NAME}/component.hpp"
        "${PARSE_OUTPUT_DIR}/include/client/${PARSE_NAME}/requests.hpp"
        "${PARSE_OUTPUT_DIR}/include/client/${PARSE_NAME}/responses.hpp"
        "${PARSE_OUTPUT_DIR}/include/client/${PARSE_NAME}/exceptions.hpp"
        "${PARSE_OUTPUT_DIR}/src/client/${PARSE_NAME}/client.cpp"
        "${PARSE_OUTPUT_DIR}/src/client/${PARSE_NAME}/client_impl.cpp"
        "${PARSE_OUTPUT_DIR}/src/client/${PARSE_NAME}/component.cpp"
        "${PARSE_OUTPUT_DIR}/src/client/${PARSE_NAME}/requests.cpp"
        "${PARSE_OUTPUT_DIR}/src/client/${PARSE_NAME}/responses.cpp"
        "${PARSE_OUTPUT_DIR}/src/client/${PARSE_NAME}/exceptions.cpp"
    )
    foreach(SCHEMA ${PARSE_SCHEMAS})
        string(REGEX REPLACE "^.*/([^/]*)\\.([^.]*)\$" "\\1" SCHEMA "${SCHEMA}")
        set(SCHEMAS ${SCHEMAS} "${PARSE_OUTPUT_DIR}/include/client/${PARSE_NAME}/${SCHEMA}.hpp"
                    "${PARSE_OUTPUT_DIR}/src/client/${PARSE_NAME}/${SCHEMA}.cpp"
        )
    endforeach()

    _userver_initialize_codegen_flag()
    add_custom_command(
        OUTPUT ${SCHEMAS}
        COMMAND env "USERVER_PYTHON=${USERVER_CHAOTIC_PYTHON_BINARY}" "${CHAOTIC_OPENAPI_BIN}" ${CHAOTIC_EXTRA_ARGS}
                ${PARSE_ARGS} --name "${PARSE_NAME}" -o "${PARSE_OUTPUT_DIR}" ${PARSE_SCHEMAS}
        COMMENT "Generating OpenAPI client ${PARSE_NAME}"
        DEPENDS ${PARSE_SCHEMAS}
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        VERBATIM ${CODEGEN}
    )
    _userver_codegen_register_files("${SCHEMAS}")
    add_library("${TARGET}" ${SCHEMAS})
    target_link_libraries("${TARGET}" userver::chaotic-openapi)
    # target_include_directories("${TARGET}" PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/")
    target_include_directories("${TARGET}" PUBLIC "${PARSE_OUTPUT_DIR}/include")
endfunction()

function(userver_target_generate_chaotic_dynamic_configs TARGET SCHEMAS_REGEX)
    file(GLOB CHGEN_FILENAMES ${SCHEMAS_REGEX})
    set(OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/dynamic_configs)

    get_property(CHAOTIC_DYNAMIC_CONFIGS_BIN GLOBAL PROPERTY userver_chaotic_dynamic_configs_bin)
    get_property(CHAOTIC_EXTRA_ARGS GLOBAL PROPERTY userver_chaotic_extra_args)
    get_property(USERVER_CHAOTIC_PYTHON_BINARY GLOBAL PROPERTY userver_chaotic_python_binary)
    get_property(CLANG_FORMAT_BIN GLOBAL PROPERTY userver_clang_format_bin)

    _userver_initialize_codegen_flag()

    set(OUTPUT_FILENAMES)
    set(CONFIG_NAMES)
    foreach(FILENAME ${CHGEN_FILENAMES})
        string(REGEX REPLACE "^(.*)/([^/]*)\\.([^.]*)\$" "\\2" SCHEMA "${FILENAME}")
        set(CONFIG_NAMES "${CONFIG_NAMES} ${SCHEMA}")

        list(
            APPEND
            OUTPUT_FILENAMES
            ${OUTPUT_DIR}/include/dynamic_config/variables/${SCHEMA}.types_fwd.hpp
            ${OUTPUT_DIR}/include/dynamic_config/variables/${SCHEMA}.types.hpp
            ${OUTPUT_DIR}/include/dynamic_config/variables/${SCHEMA}.types_parsers.ipp
            ${OUTPUT_DIR}/include/dynamic_config/variables/${SCHEMA}.hpp
            ${OUTPUT_DIR}/src/dynamic_config/variables/${SCHEMA}.types.cpp
            ${OUTPUT_DIR}/src/dynamic_config/variables/${SCHEMA}.cpp
        )
    endforeach()

    add_custom_command(
        OUTPUT ${OUTPUT_FILENAMES}
        COMMAND
            env "USERVER_PYTHON=${USERVER_CHAOTIC_PYTHON_BINARY}" "${CHAOTIC_DYNAMIC_CONFIGS_BIN}" ${CHAOTIC_EXTRA_ARGS}
            -I ${CMAKE_CURRENT_LIST_DIR}/../chaotic/include -o "${OUTPUT_DIR}" ${CHGEN_FILENAMES}
        COMMENT "Generating dynamic configs${CONFIG_NAMES}"
        DEPENDS ${CHGEN_FILENAMES}
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        VERBATIM ${CODEGEN}
    )
    _userver_codegen_register_files("${OUTPUT_FILENAMES}")
    add_library("${TARGET}" STATIC ${OUTPUT_FILENAMES})
    target_link_libraries("${TARGET}" userver::core userver::chaotic)
    target_include_directories("${TARGET}" PUBLIC "$<BUILD_INTERFACE:${OUTPUT_DIR}/include>")
endfunction()
