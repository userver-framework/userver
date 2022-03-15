project(userver-samples-testsuite-support CXX)

add_executable(${PROJECT_NAME} main.cpp now.cpp testpoint.cpp)
target_link_libraries(${PROJECT_NAME} userver-core)

userver_testsuite_add(
  NAME testsuite-${PROJECT_NAME}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests
  PYTHON_BINARY ${TESTSUITE_PYTHON_BINARY}
  PYTEST_ARGS
      --service-name=${PROJECT_NAME}
      --service-source-dir=${CMAKE_CURRENT_SOURCE_DIR}
      --service-binary=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}
)
