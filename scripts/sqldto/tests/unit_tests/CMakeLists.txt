project(sqldto-unit-tests)

add_test(
    NAME sqldto-unit
    COMMAND "${USERVER_SQLDTO_TESTS_PYTHON_BINARY}" -m pytest
            tests/unit_tests --confcutdir=tests/unit_tests
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../.."
)
