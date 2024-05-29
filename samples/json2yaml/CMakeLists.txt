cmake_minimum_required(VERSION 3.14)
project(userver-samples-json2yaml CXX)

find_package(userver COMPONENTS universal REQUIRED)

add_executable("${PROJECT_NAME}" json2yaml.cpp)
target_link_libraries("${PROJECT_NAME}" userver::universal)

# /// [add_unit_test]
add_executable(${PROJECT_NAME}-unittest json2yaml_test.cpp)

target_link_libraries(${PROJECT_NAME}-unittest
  userver::universal
  GTest::gtest_main
)
add_google_tests(${PROJECT_NAME}-unittest)
# /// [add_unit_test]

# /// [add_test]
userver_venv_setup(
  REQUIREMENTS ${CMAKE_CURRENT_SOURCE_DIR}/requirements.txt
  PYTHON_OUTPUT_VAR json2yaml_venv
)

add_test(
  NAME "${PROJECT_NAME}-pytest"
  COMMAND ${json2yaml_venv} -m pytest
    "--test-binary=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}"
    "-c /dev/null"  # Do not search for pytest.ini
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)
# /// [add_test]
