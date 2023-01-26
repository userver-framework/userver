project(userver-samples-tcp_full_duplex_service CXX)

add_executable(${PROJECT_NAME} "tcp_full_duplex_service.cpp")
target_link_libraries(${PROJECT_NAME} userver-core)

userver_testsuite_add(
  SERVICE_TARGET ${PROJECT_NAME}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests
  PYTHON_BINARY ${TESTSUITE_PYTHON_BINARY}
  PYTEST_ARGS
  --service-config=${CMAKE_CURRENT_SOURCE_DIR}/static_config.yaml
  --service-binary=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}
  --config-fallback=${CMAKE_CURRENT_SOURCE_DIR}/dynamic_config_fallback.json
  --service-port=8182
  --monitor-port=8182
)
