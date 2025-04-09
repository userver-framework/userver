project(userver-testsuite-tests)

set(TESTSUITE_TESTS
  "chaos"
  "config"
  "metrics"
  "sql_coverage_fail"
)

foreach(TEST_NAME IN LISTS TESTSUITE_TESTS)
  add_test(NAME "${PROJECT_NAME}-${TEST_NAME}"
    COMMAND "${TESTSUITE_PYTHON_BINARY}" -m pytest "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_NAME}"
    WORKING_DIRECTORY "${USERVER_ROOT_DIR}/testsuite/pytest_plugins"
  )

  set_tests_properties("${PROJECT_NAME}-${TEST_NAME}" PROPERTIES TIMEOUT 600)
endforeach()
