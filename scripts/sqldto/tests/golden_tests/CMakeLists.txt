project(sqldto-golden-tests)

add_test(
    NAME sqldto-golden
    COMMAND "${USERVER_SQLDTO_TESTS_PYTHON_BINARY}" -m pytest tests/golden_tests/golden_test.py
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../.."
)

add_custom_target(
    NAME
    sqldto-golden-canonize
    COMMAND "${USERVER_SQLDTO_CANONIZE_TESTS_PYTHON_BINARY}" -m pytest tests/golden_tests/canonize/canonize_test.py
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../.."
)
