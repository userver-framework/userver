include_guard(GLOBAL)

function(_userver_prepare_sql)
    if(NOT USERVER_DIR)
        get_filename_component(USERVER_DIR "${CMAKE_CURRENT_LIST_DIR}/userver" DIRECTORY)
    endif()
    set(USERVER_SQL_SCRIPTS_PATH "${USERVER_DIR}/scripts/sql")
    set(USERVER_SQL_DTO_SCRIPTS_PATH "${USERVER_DIR}/scripts/sqldto")

    userver_venv_setup(
        NAME userver-sql
        PYTHON_OUTPUT_VAR USERVER_SQL_PYTHON_BINARY
        REQUIREMENTS "${USERVER_SQL_SCRIPTS_PATH}/requirements.txt"
                     "${USERVER_SQL_DTO_SCRIPTS_PATH}/sqldto/generator/requirements.txt"
        UNIQUE
    )
    set_property(GLOBAL PROPERTY userver_sql_python_binary "${USERVER_SQL_PYTHON_BINARY}")
    set_property(GLOBAL PROPERTY userver_scripts_sql "${USERVER_SQL_SCRIPTS_PATH}")
    set_property(GLOBAL PROPERTY userver_scripts_sql_dto "${USERVER_SQL_DTO_SCRIPTS_PATH}")
endfunction()

_userver_prepare_sql()

# Generate C++ files with static variables of `storages::Query` type from '.sql' files with SQL queries.
#
# @arg TARGET New library target name
# @param SOURCE_DIR Source directory (defaults to ".")
# @param OUTPUT_DIR @required Directory to generate C++ files
# @param NAMESPACE @required C++ namespace to use
# @param QUERY_LOG_MODE ??? "full" or "" (defaults to "full")
# @multiparam SQL_FILES Paths to SQL files with queries
# @param DTO_DIALECT Dialect to use for DTO generation (defaults empty)
# @param MIGRATIONS_DIR Directory with .sql migration files (defaults empty)
# @param DUMP_DIR Directory to dump schema (defaults to ".")
function(userver_add_sql_library TARGET)
    set(OPTIONS)
    set(ONE_VALUE_ARGS SOURCE_DIR OUTPUT_DIR NAMESPACE QUERY_LOG_MODE DTO_DIALECT MIGRATIONS_DIR DUMP_DIR)
    set(MULTI_VALUE_ARGS SQL_FILES)
    cmake_parse_arguments(ARG "${OPTIONS}" "${ONE_VALUE_ARGS}" "${MULTI_VALUE_ARGS}" ${ARGN})
    if(NOT ARG_NAMESPACE)
        message(FATAL_ERROR "NAMESPACE argument is required")
    endif()
    if(NOT ARG_QUERY_LOG_MODE)
        set(ARG_QUERY_LOG_MODE "full")
    endif()
    set(FILENAME "sql_queries")
    if(NOT ARG_SOURCE_DIR)
        set(ARG_SOURCE_DIR ".")
    endif()
    if(NOT IS_ABSOLUTE "${ARG_SOURCE_DIR}")
        set(ARG_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${ARG_SOURCE_DIR}")
    endif()
    if(NOT ARG_DUMP_DIR)
        set(ARG_DUMP_DIR ".")
    endif()
    if(NOT IS_ABSOLUTE "${ARG_DUMP_DIR}")
        set(ARG_DUMP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${ARG_DUMP_DIR}")
    endif()
    if(ARG_MIGRATIONS_DIR AND NOT IS_ABSOLUTE "${ARG_MIGRATIONS_DIR}")
        set(ARG_MIGRATIONS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${ARG_MIGRATIONS_DIR}")
    endif()
    if(ARG_DTO_DIALECT AND NOT ARG_MIGRATIONS_DIR)
        message(FATAL_ERROR "MIGRATIONS_DIR argument is required for DTO generation")
    endif()


    set(SQL_FILES)
    foreach(WILDCARD ${ARG_SQL_FILES})
        file(
            GLOB_RECURSE FILES
            RELATIVE ${ARG_SOURCE_DIR}
            "${ARG_SOURCE_DIR}/${WILDCARD}"
        )
        list(APPEND SQL_FILES ${FILES})
    endforeach()
    list(TRANSFORM SQL_FILES PREPEND "${ARG_SOURCE_DIR}/")

    get_property(USERVER_SQL_PYTHON_BINARY GLOBAL PROPERTY userver_sql_python_binary)
    get_property(USERVER_SQL_SCRIPTS_PATH GLOBAL PROPERTY userver_scripts_sql)
    get_property(USERVER_SQL_DTO_SCRIPTS_PATH GLOBAL PROPERTY userver_scripts_sql_dto)

    set(TESTSUITE_OUTPUT_DIR ${ARG_OUTPUT_DIR}/testsuite)

    set(library_files)
    set(output_files ${ARG_OUTPUT_DIR}/include/${ARG_NAMESPACE}/${FILENAME}.hpp
                     ${ARG_OUTPUT_DIR}/src/${ARG_NAMESPACE}/${FILENAME}.cpp
                     ${TESTSUITE_OUTPUT_DIR}/sql_files.py
    )
    list(APPEND library_files ${ARG_OUTPUT_DIR}/include/${ARG_NAMESPACE}/${FILENAME}.hpp
                              ${ARG_OUTPUT_DIR}/src/${ARG_NAMESPACE}/${FILENAME}.cpp
    )

    _userver_initialize_codegen_flag()
    add_custom_command(
        OUTPUT ${output_files}
        COMMAND
            ${USERVER_SQL_PYTHON_BINARY} ${USERVER_SQL_SCRIPTS_PATH}/generator.py --namespace ${ARG_NAMESPACE}
            --source-dir ${ARG_SOURCE_DIR} --output-dir ${ARG_OUTPUT_DIR} --query-log-mode ${ARG_QUERY_LOG_MODE}
            --testsuite-output-dir ${TESTSUITE_OUTPUT_DIR} ${SQL_FILES} ${CODEGEN}
        DEPENDS ${SQL_FILES}
    )
    _userver_codegen_register_files("${output_files}")

    if(ARG_DTO_DIALECT)
        file(GLOB_RECURSE MIGRATIONS_FILES CONFIGURE_DEPENDS "${ARG_MIGRATIONS_DIR}/*.sql")

        set(output_files ${ARG_OUTPUT_DIR}/include/${ARG_NAMESPACE}/pg_models.hpp
                         ${ARG_OUTPUT_DIR}/include/${ARG_NAMESPACE}/pg_client.hpp
                         ${ARG_OUTPUT_DIR}/include/${ARG_NAMESPACE}/pg_cluster.hpp
                         ${ARG_OUTPUT_DIR}/include/${ARG_NAMESPACE}/pg_mock.hpp
                         ${ARG_OUTPUT_DIR}/src/${ARG_NAMESPACE}/pg_cluster.cpp
        )
        list(APPEND library_files ${output_files})
        _userver_initialize_codegen_flag()
        add_custom_command(
            OUTPUT ${output_files}
            COMMAND ${USERVER_SQL_PYTHON_BINARY}
                    -m sqldto.generator.main
                    --namespace "${ARG_NAMESPACE}"
                    --output-dir "${ARG_OUTPUT_DIR}"
                    --dialect "${ARG_DTO_DIALECT}"
                    --migrations-dir "${ARG_MIGRATIONS_DIR}"
                    --queries-dir "${ARG_SOURCE_DIR}"
                    --dump-dir "${ARG_DUMP_DIR}"
                    ${SQL_FILES}
                    ${CODEGEN}
            WORKING_DIRECTORY ${USERVER_SQL_DTO_SCRIPTS_PATH}
            DEPENDS ${MIGRATIONS_FILES} ${SQL_FILES}
            VERBATIM
        )
        _userver_codegen_register_files("${output_files}")
    endif()

    add_library(${TARGET} STATIC ${library_files})
    set_target_properties(${TARGET} PROPERTIES USERVER_TESTSUITE_DIRECTORY ${TESTSUITE_OUTPUT_DIR})
    target_include_directories(${TARGET} PUBLIC ${ARG_OUTPUT_DIR}/include)
    target_link_libraries(${TARGET} PUBLIC userver::core)

    if(ARG_DTO_DIALECT)
        target_link_libraries(${TARGET} PUBLIC userver::${ARG_DTO_DIALECT})
    endif()
endfunction()
