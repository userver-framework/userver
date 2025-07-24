project(userver-samples-http-client-perf CXX)

find_package(Boost REQUIRED CONFIG COMPONENTS program_options)

add_executable(${PROJECT_NAME} "httpclient_perf.cpp")
target_link_libraries(${PROJECT_NAME} userver::core Boost::program_options)

# /// [add_test]
userver_venv_setup(REQUIREMENTS ${CMAKE_CURRENT_SOURCE_DIR}/requirements.txt PYTHON_OUTPUT_VAR httperf_venv)

add_test(
    NAME "${PROJECT_NAME}-pytest"
    COMMAND "${httperf_venv}" -m pytest "--test-binary=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}"
            "-c /dev/null" # Do not search for pytest.ini
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)
# /// [add_test]
