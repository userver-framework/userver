if (NOT (CMAKE_BUILD_TYPE MATCHES "Debug" OR CMAKE_BUILD_TYPE MATCHES "Test"))
  return()
endif()
if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
  # There is not GDB for MacOS
  return()
endif()

find_program(GDB_EXECUTABLE gdb DOC "GDB executable")
if ("${GDB_EXECUTABLE}" STREQUAL GDB_EXECUTABLE-NOTFOUND)
  message(WARNING "GDB executable not found")
  return()
endif()

project(userver-gdb-tests)

set(GDB_TESTS_DIR ${CMAKE_CURRENT_LIST_DIR})
set_directory_properties(PROPERTIES TEST_PROGRAMS "" TEST_SOURCES "")

macro(add_gdb_test_program NAME SOURCE DEPS)
  project(userver-gdb-test-${NAME})

  add_executable(${PROJECT_NAME} "${SOURCE}")
  target_link_libraries(${PROJECT_NAME} ${DEPS})
  target_include_directories(${PROJECT_NAME} PRIVATE "${USERVER_ROOT_DIR}/scripts/gdb/tests/include")

  get_directory_property(TEST_PROGRAMS DIRECTORY "${GDB_TESTS_DIR}" TEST_PROGRAMS)
  get_directory_property(TEST_SOURCES DIRECTORY "${GDB_TESTS_DIR}" TEST_SOURCES)

  set_property(DIRECTORY "${GDB_TESTS_DIR}" PROPERTY TEST_PROGRAMS "${TEST_PROGRAMS}:$<TARGET_FILE:${PROJECT_NAME}>")
  set_property(DIRECTORY "${GDB_TESTS_DIR}" PROPERTY TEST_SOURCES "${TEST_SOURCES}:${CMAKE_CURRENT_LIST_DIR}/${SOURCE}")
endmacro()

add_subdirectory(src)

add_test(NAME ${PROJECT_NAME}
  COMMAND "${TESTSUITE_PYTHON_BINARY}" -m pytest
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

get_directory_property(TEST_PROGRAMS TEST_PROGRAMS)
get_directory_property(TEST_SOURCES TEST_SOURCES)

set_tests_properties(${PROJECT_NAME} PROPERTIES ENVIRONMENT "TEST_PROGRAMS=${TEST_PROGRAMS};TEST_SOURCES=${TEST_SOURCES};GDB_BIN=${GDB_EXECUTABLE}")
