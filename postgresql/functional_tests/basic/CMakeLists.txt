project(userver-postgresql-tests-basic CXX)

file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*pp")
add_executable(${PROJECT_NAME} ${SOURCES} "main.cpp")
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(${PROJECT_NAME} userver::postgresql)

userver_add_sql_library(
    ${PROJECT_NAME}_sql
    NAMESPACE postgresql_tests_basic
    SOURCE_DIR queries
    OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}
    SQL_FILES *.sql
)
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}_sql)

userver_testsuite_add_simple(SQL_LIBRARY ${PROJECT_NAME}_sql WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests")

# For internal userver tests only.
# Functional test that starts the service through its `start-*` cmake target
# (see tests-start-target/conftest.py) instead of spawning the binary directly.
userver_testsuite_add_simple(
    TEST_SUFFIX start-target
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests-start-target"
    # Makes the shared `start_target.pytest_plugin` plugin importable.
    PYTHONPATH "${USERVER_ROOT_DIR}/scripts/tests"
    PYTEST_ARGS --cmake-command "${CMAKE_COMMAND}" --cmake-build-dir "${CMAKE_BINARY_DIR}" --service-start-target
                "start-${PROJECT_NAME}"
)
# The service is started on its fixed static-config port, so the test must
# not run concurrently with others that could occupy the same port.
set_tests_properties("testsuite-${PROJECT_NAME}-start-target" PROPERTIES RUN_SERIAL TRUE)
