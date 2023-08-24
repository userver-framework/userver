project(userver-samples-testsuite-support CXX)

file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} userver-core)

# /// [testsuite - cmake]
userver_testsuite_add(
  SERVICE_TARGET ${PROJECT_NAME}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests
  PYTHON_BINARY ${TESTSUITE_PYTHON_BINARY}
  PYTEST_ARGS
  --service-config=${CMAKE_CURRENT_SOURCE_DIR}/static_config.yaml
  --service-binary=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}
  --config-fallback=${CMAKE_CURRENT_SOURCE_DIR}/dynamic_config_fallback.json
  --service-config-vars=${CMAKE_CURRENT_SOURCE_DIR}/config_vars.yaml
)
# /// [testsuite - cmake]
