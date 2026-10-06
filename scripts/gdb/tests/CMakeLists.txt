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
set_directory_properties(
    PROPERTIES TEST_PROGRAMS_core ""
               TEST_PROGRAMS_universal ""
               TEST_SOURCES_core ""
               TEST_SOURCES_universal ""
               TEST_TARGETS_core ""
               TEST_TARGETS_universal ""
)

macro(add_gdb_test_program GROUP NAME SOURCE DEPS)
    project(userver-gdb-test-${NAME})

    add_executable(${PROJECT_NAME} "${SOURCE}")
    target_link_libraries(${PROJECT_NAME} PUBLIC userver-gdb-tests-include ${DEPS})
    target_include_directories(
        ${PROJECT_NAME}
        PRIVATE "${USERVER_ROOT_DIR}/scripts/gdb/tests/include"
        PRIVATE "${USERVER_ROOT_DIR}/scripts/gdb/include"
    )

    get_directory_property(TEST_PROGRAMS DIRECTORY "${GDB_TESTS_DIR}" TEST_PROGRAMS_${GROUP})
    get_directory_property(TEST_SOURCES DIRECTORY "${GDB_TESTS_DIR}" TEST_SOURCES_${GROUP})

    set_property(
        DIRECTORY "${GDB_TESTS_DIR}"
        PROPERTY TEST_PROGRAMS_${GROUP} "${TEST_PROGRAMS}:$<TARGET_FILE:${PROJECT_NAME}>"
    )
    set_property(
        DIRECTORY "${GDB_TESTS_DIR}"
        PROPERTY TEST_SOURCES_${GROUP} "${TEST_SOURCES}:${CMAKE_CURRENT_LIST_DIR}/${SOURCE}"
    )
    set_property(DIRECTORY "${GDB_TESTS_DIR}" APPEND PROPERTY TEST_TARGETS_${GROUP} ${PROJECT_NAME})
endmacro()

add_subdirectory(include)
add_subdirectory(src)

get_target_property(USERVER_GDB_NAMESPACE userver-universal USERVER_GDB_NAMESPACE)

function(add_gdb_test_suite GROUP)
    get_directory_property(TEST_PROGRAMS TEST_PROGRAMS_${GROUP})
    get_directory_property(TEST_SOURCES TEST_SOURCES_${GROUP})
    get_directory_property(TEST_TARGETS TEST_TARGETS_${GROUP})

    if(GROUP STREQUAL "core")
        set(TESTS_IN_RELEASE gdb_test_utask)
    elseif(GROUP STREQUAL "universal")
        set(TESTS_IN_RELEASE gdb_test_json:gdb_test_yaml:gdb_test_fast_pimpl)
    else()
        message(FATAL_ERROR "Unknown GDB test suite group '${GROUP}'")
    endif()

    set(COMPONENT_UNITTEST_TARGET userver-${GROUP}-unittest)
    if(TARGET ${COMPONENT_UNITTEST_TARGET})
        add_dependencies(${COMPONENT_UNITTEST_TARGET} ${TEST_TARGETS})
    endif()

    set(TEST_FILES "${CMAKE_CURRENT_SOURCE_DIR}/test_printers.py")
    if(GROUP STREQUAL "universal")
        list(APPEND TEST_FILES "${CMAKE_CURRENT_SOURCE_DIR}/test_gen_gdb_printers_tests.py")
    endif()

    set(TEST_NAME userver-${GROUP}-gdb-tests)
    add_test(
        NAME ${TEST_NAME}
        COMMAND "${TESTSUITE_PYTHON_BINARY}" -m pytest ${TEST_FILES}
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )

    set(
        TEST_ENVIRONMENT
        "TEST_PROGRAMS=${TEST_PROGRAMS}"
        "TEST_SOURCES=${TEST_SOURCES}"
        "BUILD_TYPE=$<CONFIG>"
        "GDB_BIN=${GDB_EXECUTABLE}"
        "TESTS_IN_RELEASE=${TESTS_IN_RELEASE}"
        "USERVER_NAMESPACE=${USERVER_GDB_NAMESPACE}"
    )
    if(GROUP STREQUAL "core")
        list(APPEND TEST_ENVIRONMENT "TESTS_COREDUMP=gdb_test_utask")
    endif()
    set_tests_properties(${TEST_NAME} PROPERTIES ENVIRONMENT "${TEST_ENVIRONMENT}")
endfunction()

add_gdb_test_suite(universal)
if(USERVER_FEATURE_CORE)
    add_gdb_test_suite(core)
endif()
