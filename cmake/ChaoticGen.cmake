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
    # @ingroup compilation
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
    set_property(GLOBAL PROPERTY userver_chaotic_scripts_path "${USERVER_CHAOTIC_SCRIPTS_PATH}")
endfunction()

_userver_prepare_chaotic()

# Generates ${TARGET} cmake target for C++ types, parsers, serializers from JSONSchema file(s).
#
# @arg TARGET smth
# @param OUTPUT_DIR - where to put generated .cpp/.hpp/.ipp files, usually ${CMAKE_CURRENT_BINARY_DIR}/smth
# @param RELATIVE_TO - --relative-to option to chaotic-gen
# @param FORMAT - can be ON/OFF, enable to format generated files, defaults to USERVER_CHAOTIC_FORMAT
# @multiparam SCHEMAS - JSONSchema source files
# @param INSTALL_INCLUDES_COMPONENT - component to install generated includes
# @multiparam LINK_TARGETS - targets to link (used by x-usrv-cpp-type)
# @option NO_SAX_PARSE - Do not generate SAX parser and efficient FromJsonString() member factory function
# @option NO_STREAM_WRITER - Do not generate streaming serializers via WriteToStream()
function(userver_target_generate_chaotic TARGET)
    set(OPTIONS GENERATE_SERIALIZERS PARSE_EXTRA_FORMATS NO_SAX_PARSE NO_STREAM_WRITER)
    set(ONE_VALUE_ARGS OUTPUT_DIR RELATIVE_TO FORMAT INSTALL_INCLUDES_COMPONENT OUTPUT_PREFIX ERASE_PATH_PREFIX)
    set(MULTI_VALUE_ARGS SCHEMAS LAYOUT INCLUDE_DIRS LINK_TARGETS)
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

        if(NOT PARSE_NO_SAX_PARSE)
            list(
                APPEND
                SCHEMAS
                "${PARSE_OUTPUT_DIR}/${PARSE_OUTPUT_PREFIX}/${SCHEMA}_sax_parsers.hpp"
            )
        endif()
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
    
    if(PARSE_NO_SAX_PARSE)
        list(APPEND CHAOTIC_ARGS "--no-sax-parse")
    endif()

    if(PARSE_NO_STREAM_WRITER)
        list(APPEND CHAOTIC_ARGS "--no-stream-writer")
    endif()

    if(PARSE_ERASE_PATH_PREFIX)
        list(APPEND CHAOTIC_ARGS "-e" "${PARSE_ERASE_PATH_PREFIX}")
    endif()

    list(APPEND CHAOTIC_ARGS "-o" "${PARSE_OUTPUT_DIR}/${PARSE_OUTPUT_PREFIX}")
    list(APPEND CHAOTIC_ARGS "--relative-to" "${PARSE_RELATIVE_TO}")

    _userver_initialize_codegen_flag()

    add_library("${TARGET}" ${SCHEMAS})
    target_link_libraries("${TARGET}" PUBLIC userver::chaotic ${PARSE_LINK_TARGETS})

    get_target_property(TARGET_LIBRARIES "${TARGET}" LINK_LIBRARIES)
    foreach(LIBRARY ${TARGET_LIBRARIES})
        get_target_property(DIRS "${LIBRARY}" INTERFACE_INCLUDE_DIRECTORIES)
        foreach(DIRECTORY ${DIRS})
            set(CHAOTIC_ARGS ${CHAOTIC_ARGS} -I ${DIRECTORY})
        endforeach()
    endforeach()

    add_custom_command(
        OUTPUT ${SCHEMAS}
        COMMAND ${CMAKE_COMMAND} -E env "USERVER_PYTHON=${USERVER_CHAOTIC_PYTHON_BINARY}" "${CHAOTIC_BIN}"
                ${CHAOTIC_EXTRA_ARGS} ${CHAOTIC_ARGS} ${PARSE_SCHEMAS} --clang-format "${CLANG_FORMAT}"
        DEPENDS ${PARSE_SCHEMAS}
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        VERBATIM ${CODEGEN}
    )
    _userver_codegen_register_files("${SCHEMAS}")
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

# TODO
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
        "${PARSE_OUTPUT_DIR}/include/clients/${PARSE_NAME}/client.hpp"
        "${PARSE_OUTPUT_DIR}/include/clients/${PARSE_NAME}/client_impl.hpp"
        "${PARSE_OUTPUT_DIR}/include/clients/${PARSE_NAME}/component.hpp"
        "${PARSE_OUTPUT_DIR}/include/clients/${PARSE_NAME}/requests.hpp"
        "${PARSE_OUTPUT_DIR}/include/clients/${PARSE_NAME}/responses.hpp"
        "${PARSE_OUTPUT_DIR}/include/clients/${PARSE_NAME}/exceptions.hpp"
        "${PARSE_OUTPUT_DIR}/src/clients/${PARSE_NAME}/client.cpp"
        "${PARSE_OUTPUT_DIR}/src/clients/${PARSE_NAME}/client_impl.cpp"
        "${PARSE_OUTPUT_DIR}/src/clients/${PARSE_NAME}/component.cpp"
        "${PARSE_OUTPUT_DIR}/src/clients/${PARSE_NAME}/requests.cpp"
        "${PARSE_OUTPUT_DIR}/src/clients/${PARSE_NAME}/responses.cpp"
        "${PARSE_OUTPUT_DIR}/src/clients/${PARSE_NAME}/exceptions.cpp"
    )
    foreach(SCHEMA ${PARSE_SCHEMAS})
        string(REGEX REPLACE "^.*/([^/]*)\\.([^.]*)\$" "\\1" SCHEMA "${SCHEMA}")
        set(SCHEMAS ${SCHEMAS} "${PARSE_OUTPUT_DIR}/include/clients/${PARSE_NAME}/${SCHEMA}.hpp"
                    "${PARSE_OUTPUT_DIR}/src/clients/${PARSE_NAME}/${SCHEMA}.cpp"
        )
    endforeach()

    _userver_initialize_codegen_flag()
    add_custom_command(
        OUTPUT ${SCHEMAS}
        COMMAND ${CMAKE_COMMAND} -E env "USERVER_PYTHON=${USERVER_CHAOTIC_PYTHON_BINARY}" "${CHAOTIC_OPENAPI_BIN}" ${CHAOTIC_EXTRA_ARGS}
                --gen client ${PARSE_ARGS} --name "${PARSE_NAME}" -o "${PARSE_OUTPUT_DIR}" ${PARSE_SCHEMAS}
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

# Generates ${TARGET} cmake target for C++ server-side handler code from OpenAPI YAML file(s).
#
# @arg TARGET target name
# @param NAME - service name (--name); also used in include/src paths under handlers/NAME/
# @param OUTPUT_DIR - where to put generated include/ and src/ trees
# @param SRC_DIR - parent directory of handlers/NAME/; view stubs are written here (skipped if already present)
# @multiparam SCHEMAS - OpenAPI YAML source files
# @multiparam ARGS - extra arguments passed to chaotic-openapi-gen
function(userver_target_generate_openapi_handlers TARGET)
    set(OPTIONS)
    set(ONE_VALUE_ARGS NAME OUTPUT_DIR SRC_DIR)
    set(MULTI_VALUE_ARGS SCHEMAS ARGS)
    cmake_parse_arguments(PARSE "${OPTIONS}" "${ONE_VALUE_ARGS}" "${MULTI_VALUE_ARGS}" ${ARGN})

    get_property(CHAOTIC_OPENAPI_BIN GLOBAL PROPERTY userver_chaotic_openapi_bin)
    get_property(CHAOTIC_EXTRA_ARGS GLOBAL PROPERTY userver_chaotic_extra_args)
    get_property(USERVER_CHAOTIC_PYTHON_BINARY GLOBAL PROPERTY userver_chaotic_python_binary)
    get_property(USERVER_CHAOTIC_SCRIPTS_PATH GLOBAL PROPERTY userver_chaotic_scripts_path)

    if(NOT PARSE_NAME)
        message(FATAL_ERROR "NAME is required")
    endif()

    if(NOT PARSE_SCHEMAS)
        message(FATAL_ERROR "SCHEMAS is required")
    endif()

    if(NOT DEFINED PARSE_OUTPUT_DIR)
        set(PARSE_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/${PARSE_NAME}")
    endif()
    file(MAKE_DIRECTORY "${PARSE_OUTPUT_DIR}")

    # Extract operation relpaths from YAML at configure time so we can enumerate output files.
    execute_process(
        COMMAND
            "${USERVER_CHAOTIC_PYTHON_BINARY}" "${USERVER_CHAOTIC_SCRIPTS_PATH}/openapi_operations.py"
            ${PARSE_SCHEMAS}
        OUTPUT_VARIABLE _OPERATIONS
        OUTPUT_STRIP_TRAILING_WHITESPACE
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        RESULT_VARIABLE _OPERATIONS_RESULT
    )
    if(NOT _OPERATIONS_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to extract operation names from OpenAPI YAML: ${_OPERATIONS_RESULT}")
    endif()

    set(_HANDLER_FILES)
    foreach(OP ${_OPERATIONS})
        set(_OP_INC "${PARSE_OUTPUT_DIR}/include/handlers/${PARSE_NAME}/${OP}")
        set(_OP_SRC "${PARSE_OUTPUT_DIR}/src/handlers/${PARSE_NAME}/${OP}")
        list(
            APPEND _HANDLER_FILES
            "${_OP_INC}/handler.hpp"
            "${_OP_INC}/requests.hpp"
            "${_OP_INC}/responses.hpp"
            "${_OP_SRC}/handler.cpp"
            "${_OP_SRC}/requests.cpp"
            "${_OP_SRC}/responses.cpp"
        )
    endforeach()

    set(_SCHEMA_TYPE_FILES)
    foreach(SCHEMA ${PARSE_SCHEMAS})
        string(REGEX REPLACE "^.*/([^/]*)\\.([^.]*)\$" "\\1" _SCHEMA_STEM "${SCHEMA}")
        list(
            APPEND _SCHEMA_TYPE_FILES
            "${PARSE_OUTPUT_DIR}/include/handlers/${PARSE_NAME}/${_SCHEMA_STEM}.hpp"
            "${PARSE_OUTPUT_DIR}/src/handlers/${PARSE_NAME}/${_SCHEMA_STEM}.cpp"
        )
    endforeach()

    set(_GEN_MODE "handlers")
    # New view stubs that don't exist yet — declared as outputs of the command.
    set(_NEW_VIEW_FILES)
    # All view sources (existing or new) — compiled into the library.
    set(_ALL_VIEW_SRCS)
    if(PARSE_SRC_DIR)
        set(_GEN_MODE "handlers+views")
        foreach(OP ${_OPERATIONS})
            set(_OP_VIEW "${PARSE_SRC_DIR}/handlers/${PARSE_NAME}/${OP}")
            set(_VIEW_HPP "${_OP_VIEW}/view.hpp")
            set(_VIEW_CPP "${_OP_VIEW}/view.cpp")
            # Only declare view stubs as outputs if they don't already exist.
            # save_views() skips existing files; listing them as outputs for
            # cmake causes a rebuild cycle when the generator leaves them intact.
            if(NOT EXISTS "${_VIEW_HPP}")
                list(APPEND _NEW_VIEW_FILES "${_VIEW_HPP}")
            endif()
            if(NOT EXISTS "${_VIEW_CPP}")
                list(APPEND _NEW_VIEW_FILES "${_VIEW_CPP}")
            endif()
            list(APPEND _ALL_VIEW_SRCS "${_VIEW_CPP}")
        endforeach()
    endif()

    set(_CONFIG_YAML_PATH "${PARSE_OUTPUT_DIR}/config.chaotic.yaml")
    set(_ALL_OUTPUTS ${_HANDLER_FILES} ${_SCHEMA_TYPE_FILES} ${_NEW_VIEW_FILES} "${_CONFIG_YAML_PATH}")

    set(_GEN_ARGS
        --name "${PARSE_NAME}"
        --gen "${_GEN_MODE}"
        -o "${PARSE_OUTPUT_DIR}"
        ${PARSE_ARGS}
    )
    if(PARSE_SRC_DIR)
        list(APPEND _GEN_ARGS --src-dir "${PARSE_SRC_DIR}/handlers/${PARSE_NAME}")
    endif()

    _userver_initialize_codegen_flag()
    add_custom_command(
        OUTPUT ${_ALL_OUTPUTS}
        COMMAND
            ${CMAKE_COMMAND} -E env "USERVER_PYTHON=${USERVER_CHAOTIC_PYTHON_BINARY}"
            "${CHAOTIC_OPENAPI_BIN}" ${CHAOTIC_EXTRA_ARGS} ${_GEN_ARGS} ${PARSE_SCHEMAS}
        COMMENT "Generating OpenAPI handlers ${PARSE_NAME}"
        DEPENDS ${PARSE_SCHEMAS}
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        VERBATIM ${CODEGEN}
    )
    _userver_codegen_register_files("${_ALL_OUTPUTS}")

    add_library("${TARGET}" ${_HANDLER_FILES} ${_SCHEMA_TYPE_FILES} ${_ALL_VIEW_SRCS})
    target_link_libraries("${TARGET}" userver::chaotic-openapi)
    target_include_directories("${TARGET}" PUBLIC "${PARSE_OUTPUT_DIR}/include")
    if(PARSE_SRC_DIR)
        target_include_directories("${TARGET}" PUBLIC "${PARSE_SRC_DIR}")
    endif()
    set_property(TARGET "${TARGET}" APPEND PROPERTY
        INTERFACE_USERVER_EXTRA_CONFIG_YAML_PATHS "${_CONFIG_YAML_PATH}")
endfunction()

# Merges BASE_CONFIGS (user-provided, applied last / highest priority) with
# config.chaotic.yaml files collected transitively from BINARY_TARGET's link graph.
# Writes the merged result to OUTPUT.
#
# @arg BINARY_TARGET  - executable/library that links handler targets
# @multiparam BASE_CONFIGS - user config yaml files (applied last, highest priority)
# @param OUTPUT       - path for the final merged config.yaml
function(userver_generate_config_yaml BINARY_TARGET)
    cmake_parse_arguments(PARSE "" "OUTPUT" "BASE_CONFIGS" ${ARGN})

    if(NOT PARSE_OUTPUT)
        message(FATAL_ERROR "OUTPUT is required")
    endif()

    get_property(USERVER_CHAOTIC_PYTHON_BINARY GLOBAL PROPERTY userver_chaotic_python_binary)
    get_property(USERVER_CHAOTIC_SCRIPTS_PATH GLOBAL PROPERTY userver_chaotic_scripts_path)

    _userver_collect_extra_config_yamls("${BINARY_TARGET}" _extra_configs)

    add_custom_command(
        OUTPUT "${PARSE_OUTPUT}"
        COMMAND
            "${USERVER_CHAOTIC_PYTHON_BINARY}"
            "${USERVER_CHAOTIC_SCRIPTS_PATH}/merge_yaml_configs/main.py"
            ${_extra_configs}
            ${PARSE_BASE_CONFIGS}
            -o "${PARSE_OUTPUT}"
        DEPENDS ${_extra_configs} ${PARSE_BASE_CONFIGS}
        COMMENT "Merging config.yaml for ${BINARY_TARGET}"
        VERBATIM
    )
    add_custom_target("${BINARY_TARGET}_config" ALL DEPENDS "${PARSE_OUTPUT}")
endfunction()

function(_userver_collect_extra_config_yamls_impl TARGET)
    get_property(_visited GLOBAL PROPERTY _userver_config_yaml_visited)
    if("${TARGET}" IN_LIST _visited)
        return()
    endif()
    set_property(GLOBAL APPEND PROPERTY _userver_config_yaml_visited "${TARGET}")

    get_target_property(_prop "${TARGET}" INTERFACE_USERVER_EXTRA_CONFIG_YAML_PATHS)
    if(_prop)
        set_property(GLOBAL APPEND PROPERTY _userver_config_yaml_result "${_prop}")
    endif()

    get_target_property(_deps "${TARGET}" LINK_LIBRARIES)
    if(_deps)
        foreach(_dep IN LISTS _deps)
            if(TARGET "${_dep}")
                _userver_collect_extra_config_yamls_impl("${_dep}")
            endif()
        endforeach()
    endif()
endfunction()

function(_userver_collect_extra_config_yamls TARGET RESULT_VAR)
    set_property(GLOBAL PROPERTY _userver_config_yaml_result "")
    set_property(GLOBAL PROPERTY _userver_config_yaml_visited "")
    _userver_collect_extra_config_yamls_impl("${TARGET}")
    get_property(_result GLOBAL PROPERTY _userver_config_yaml_result)
    set(${RESULT_VAR} "${_result}" PARENT_SCOPE)
endfunction()

#TODO
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
        COMMAND ${CMAKE_COMMAND} -E env "USERVER_PYTHON=${USERVER_CHAOTIC_PYTHON_BINARY}" "${CHAOTIC_DYNAMIC_CONFIGS_BIN}"
                ${CHAOTIC_EXTRA_ARGS} -o "${OUTPUT_DIR}" ${CHGEN_FILENAMES}
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
