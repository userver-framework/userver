project(userver-clickhouse-tests CXX)

add_custom_target(${PROJECT_NAME})

function(userver_chaos_testsuite_add)
    userver_testsuite_add(
      SERVICE_TARGET ${PROJECT_NAME}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests
      PYTHON_BINARY ${TESTSUITE_PYTHON_BINARY}
      PYTEST_ARGS
      --service-config=${CMAKE_CURRENT_SOURCE_DIR}/static_config.yaml
      --service-source-dir=${CMAKE_CURRENT_SOURCE_DIR}
      --service-binary=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}
      --config-fallback=${CMAKE_CURRENT_SOURCE_DIR}/dynamic_config_fallback.json
      ${PYTEST_ARGS_CONFIG_VARS}
    )
endfunction()

add_subdirectory(basic_chaos)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-basic-chaos)

add_subdirectory(metrics)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-metrics)
