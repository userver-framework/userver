if(USERVER_SANITIZE)
    return() # boost ucontext switching is not supported yet
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    # There is not GDB for MacOS
    return()
endif()

find_program(GDB_EXECUTABLE gdb DOC "GDB executable")
if("${GDB_EXECUTABLE}" STREQUAL GDB_EXECUTABLE-NOTFOUND)
    message(WARNING "GDB executable not found")
    return()
endif()

macro(_check_clang_and_gdb_compatibility)
    execute_process(
        COMMAND ${GDB_EXECUTABLE} --version
        OUTPUT_VARIABLE GDB_VERSION_OUTPUT
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    string(REGEX MATCH "([0-9]+\\.[0-9]+)" GDB_VERSION_MATCH "${GDB_VERSION_OUTPUT}")
    if(GDB_VERSION_MATCH)
        set(GDB_VERSION ${CMAKE_MATCH_1})
    else()
        message(WARNING "Could not determine GDB version")
        set(GDB_VERSION 0)
    endif()

    string(REGEX MATCH "^([0-9]+)" GDB_VERSION_MAJOR "${GDB_VERSION}")

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang"
       AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 16.0.0
       AND GDB_VERSION_MAJOR LESS 14.0
    )
        message(WARNING "GDB tests disabled: Update your gdb version (${GDB_VERSION}) for >= 14.0 "
                        "or downgrade your clang version (${CMAKE_CXX_COMPILER_VERSION}) for < 16.0.0"
        )
        return()
    endif()
endmacro()

_check_clang_and_gdb_compatibility()

project(userver-gdb-tests)

set(GDB_TESTS_DIR ${CMAKE_CURRENT_LIST_DIR})
set_directory_properties(PROPERTIES TEST_PROGRAMS "" TEST_SOURCES "")

macro(add_gdb_test_program NAME SOURCE DEPS)
    project(userver-gdb-test-${NAME})

    add_executable(${PROJECT_NAME} "${SOURCE}")
    target_link_libraries(${PROJECT_NAME} userver-gdb-tests-include ${DEPS})
    target_include_directories(
        ${PROJECT_NAME}
        PRIVATE "${USERVER_ROOT_DIR}/scripts/gdb/tests/include"
        PRIVATE "${USERVER_ROOT_DIR}/scripts/gdb/include"
    )

    get_directory_property(TEST_PROGRAMS DIRECTORY "${GDB_TESTS_DIR}" TEST_PROGRAMS)
    get_directory_property(TEST_SOURCES DIRECTORY "${GDB_TESTS_DIR}" TEST_SOURCES)

    set_property(DIRECTORY "${GDB_TESTS_DIR}" PROPERTY TEST_PROGRAMS "${TEST_PROGRAMS}:$<TARGET_FILE:${PROJECT_NAME}>")
    set_property(
        DIRECTORY "${GDB_TESTS_DIR}" PROPERTY TEST_SOURCES "${TEST_SOURCES}:${CMAKE_CURRENT_LIST_DIR}/${SOURCE}"
    )
endmacro()

add_subdirectory(include)
add_subdirectory(src)

add_test(
    NAME ${PROJECT_NAME}
    COMMAND "${TESTSUITE_PYTHON_BINARY}" -m pytest
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

get_directory_property(TEST_PROGRAMS TEST_PROGRAMS)
get_directory_property(TEST_SOURCES TEST_SOURCES)

set_tests_properties(
    ${PROJECT_NAME}
    PROPERTIES
        ENVIRONMENT
        "TEST_PROGRAMS=${TEST_PROGRAMS};TEST_SOURCES=${TEST_SOURCES};GDB_BIN=${GDB_EXECUTABLE};TESTS_COREDUMP=userver-gdb-test-utask"
)
