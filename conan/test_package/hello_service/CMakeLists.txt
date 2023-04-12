project(conan-samples-hello_service CXX)

add_executable(${PROJECT_NAME} "hello_service.cpp")
target_link_libraries(${PROJECT_NAME} userver::core)

userver_testsuite_add(
  SERVICE_TARGET ${PROJECT_NAME}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  REQUIREMENTS ${CMAKE_CURRENT_SOURCE_DIR}/requirements.txt
  PYTEST_ARGS
  --service-config=${CMAKE_CURRENT_SOURCE_DIR}/static_config.yaml
  --service-source-dir=${CMAKE_CURRENT_SOURCE_DIR}
  --service-binary=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}
  --config-fallback=${CMAKE_CURRENT_SOURCE_DIR}/dynamic_config_fallback.json
)

