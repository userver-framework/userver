include_guard(GLOBAL)

function(_userver_prepare_sql)
    if(NOT USERVER_DIR)
        get_filename_component(USERVER_DIR "${CMAKE_CURRENT_LIST_DIR}/userver" DIRECTORY)
    endif()
    set(USERVER_SQL_SCRIPTS_PATH "${USERVER_DIR}/scripts/sql")

    userver_venv_setup(
        NAME userver-sql
        PYTHON_OUTPUT_VAR USERVER_SQL_PYTHON_BINARY
        REQUIREMENTS "${USERVER_SQL_SCRIPTS_PATH}/requirements.txt"
        UNIQUE
    )
    set_property(GLOBAL PROPERTY userver_sql_python_binary "${USERVER_SQL_PYTHON_BINARY}")
    set_property(GLOBAL PROPERTY userver_scripts_sql "${USERVER_SQL_SCRIPTS_PATH}")
endfunction()

_userver_prepare_sql()

function(userver_add_sql_library TARGET)
    set(OPTIONS)
    set(ONE_VALUE_ARGS OUTPUT_DIR NAMESPACE QUERY_LOG_MODE)
    set(MULTI_VALUE_ARGS SQL_FILES)
    cmake_parse_arguments(ARG "${OPTIONS}" "${ONE_VALUE_ARGS}" "${MULTI_VALUE_ARGS}" ${ARGN})
    if(NOT ARG_NAMESPACE)
        message(FATAL_ERROR "NAMESPACE argument is required")
    endif()
    if(NOT ARG_QUERY_LOG_MODE)
        set(ARG_QUERY_LOG_MODE "full")
    endif()
    set(FILENAME "sql_queries")

    set(SQL_FILES)
    foreach(WILDCARD ${ARG_SQL_FILES})
        file(GLOB FILES ${WILDCARD})
        list(APPEND SQL_FILES ${FILES})
    endforeach()

    get_property(USERVER_SQL_PYTHON_BINARY GLOBAL PROPERTY userver_sql_python_binary)
    get_property(USERVER_SQL_SCRIPTS_PATH GLOBAL PROPERTY userver_scripts_sql)

    set(TESTSUITE_OUTPUT_DIR ${ARG_OUTPUT_DIR}/testsuite)

    set(output_files ${ARG_OUTPUT_DIR}/include/${ARG_NAMESPACE}/${FILENAME}.hpp
                     ${ARG_OUTPUT_DIR}/src/${ARG_NAMESPACE}/${FILENAME}.cpp ${TESTSUITE_OUTPUT_DIR}/sql_files.py
    )
    _userver_initialize_codegen_flag()
    add_custom_command(
        OUTPUT ${output_files}
        COMMAND
            ${USERVER_SQL_PYTHON_BINARY} ${USERVER_SQL_SCRIPTS_PATH}/generator.py --namespace ${ARG_NAMESPACE}
            --output-dir ${ARG_OUTPUT_DIR} --query-log-mode ${ARG_QUERY_LOG_MODE} --testsuite-output-dir
            ${TESTSUITE_OUTPUT_DIR} ${SQL_FILES} ${CODEGEN}
        DEPENDS ${SQL_FILES}
    )
    _userver_codegen_register_files("${output_files}")

    add_library(
        ${TARGET} STATIC ${ARG_OUTPUT_DIR}/src/${ARG_NAMESPACE}/${FILENAME}.cpp
                         ${ARG_OUTPUT_DIR}/include/${ARG_NAMESPACE}/${FILENAME}.hpp
    )
    set_target_properties(${TARGET} PROPERTIES USERVER_TESTSUITE_DIRECTORY ${TESTSUITE_OUTPUT_DIR})
    target_include_directories(${TARGET} PUBLIC ${ARG_OUTPUT_DIR}/include)
    target_link_libraries(${TARGET} PUBLIC userver::postgresql)
endfunction()
