project(userver-grpc-tests CXX)

add_custom_target(${PROJECT_NAME})

function(userver_chaos_testsuite_add)
  set(options)
  set(oneValueArgs TESTS_DIRECTORY PROTO_PATH)
  set(multiValueArgs)
  cmake_parse_arguments(
      ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if (NOT ARG_TESTS_DIRECTORY)
    set(ARG_TESTS_DIRECTORY tests)
  endif()
  if (NOT ARG_PROTO_PATH)
    set(ARG_PROTO_PATH "${CMAKE_CURRENT_BINARY_DIR}/proto")
  endif()

  set(CONFIG_VARS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/config_vars.yaml")
  if (EXISTS "${CONFIG_VARS_PATH}")
    set(PYTEST_ARGS_CONFIG_VARS "--service-config-vars=${CONFIG_VARS_PATH}")
  else()
    set(PYTEST_ARGS_CONFIG_VARS "")
  endif()
  userver_testsuite_add(
      SERVICE_TARGET ${PROJECT_NAME}-${ARG_TESTS_DIRECTORY}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${ARG_TESTS_DIRECTORY}
      PYTHON_BINARY ${TESTSUITE_PYTHON_BINARY}
      PYTHONPATH ${ARG_PROTO_PATH}
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
