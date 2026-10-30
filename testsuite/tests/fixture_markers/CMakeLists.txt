add_test(
    NAME "${PROJECT_NAME}-fixture-markers"
    COMMAND
        "${TESTSUITE_PYTHON_BINARY}" -m pytest
        "${CMAKE_CURRENT_SOURCE_DIR}"
        -o "pythonpath=${USERVER_ROOT_DIR}/testsuite/tests"
    WORKING_DIRECTORY "${USERVER_ROOT_DIR}/testsuite/pytest_plugins"
)

set_tests_properties("${PROJECT_NAME}-fixture-markers" PROPERTIES TIMEOUT 600)
