project(userver-core-tests-static-service CXX)

add_executable(${PROJECT_NAME} "main.cpp")
target_link_libraries(${PROJECT_NAME} userver::core)

userver_testsuite_add_simple(WORKING_DIRECTORY tests-serve-from-root)
userver_testsuite_add_simple(WORKING_DIRECTORY tests-serve-from-subpath)

# Functional test that starts the service through its `start-*` cmake target
# (see tests-start-target/conftest.py) instead of spawning the binary directly. It
# drives the service launched by the `tests-serve-from-root` runner, whose
# config hook serves the `public/` directory at the root.
userver_testsuite_add_simple(
    TEST_SUFFIX start-target
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests-start-target"
    # Makes the shared `start_target.pytest_plugin` plugin importable.
    PYTHONPATH "${USERVER_ROOT_DIR}/scripts/tests"
    PYTEST_ARGS --cmake-command "${CMAKE_COMMAND}" --cmake-build-dir "${CMAKE_BINARY_DIR}" --service-start-target
                "start-${PROJECT_NAME}-tests-serve-from-root"
)
# The service is started on its fixed static-config port, so the test must
# not run concurrently with others that could occupy the same port.
set_tests_properties("testsuite-${PROJECT_NAME}-start-target" PROPERTIES RUN_SERIAL TRUE)
