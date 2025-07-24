project(userver-samples-dns-resolver CXX)

find_package(Boost REQUIRED CONFIG COMPONENTS program_options)

add_executable(${PROJECT_NAME} "resolver.cpp")
target_link_libraries(${PROJECT_NAME} userver::core Boost::program_options)

userver_venv_setup(REQUIREMENTS ${CMAKE_CURRENT_SOURCE_DIR}/requirements.txt PYTHON_OUTPUT_VAR dnsresolv_venv)
add_test(
    NAME "${PROJECT_NAME}-pytest"
    COMMAND "${dnsresolv_venv}" -m pytest "--test-binary=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}"
            "-c /dev/null" # Do not search for pytest.ini
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)
