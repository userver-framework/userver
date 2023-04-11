project(userver-rabbitmq-tests CXX)

add_custom_target(${PROJECT_NAME})

function(userver_chaos_testsuite_add)
    set(options)
    set(oneValueArgs TESTS_DIRECTORY)
    set(multiValueArgs)
    cmake_parse_arguments(
        ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    if (NOT ARG_TESTS_DIRECTORY)
        set(ARG_TESTS_DIRECTORY tests)
    endif()

    userver_testsuite_add(
      SERVICE_TARGET ${PROJECT_NAME}-${ARG_TESTS_DIRECTORY}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${ARG_TESTS_DIRECTORY}
      PYTHON_BINARY ${TESTSUITE_PYTHON_BINARY}
      PYTEST_ARGS
      --service-config=${CMAKE_CURRENT_SOURCE_DIR}/static_config.yaml
      --service-source-dir=${CMAKE_CURRENT_SOURCE_DIR}
      --service-binary=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}
      --config-fallback=${CMAKE_CURRENT_SOURCE_DIR}/dynamic_config_fallback.json
      ${PYTEST_ARGS_CONFIG_VARS}
    )

    set_tests_properties(testsuite-${PROJECT_NAME}-${ARG_TESTS_DIRECTORY}
      PROPERTIES ENVIRONMENT "TESTSUITE_RABBITMQ_SERVER_START_TIMEOUT=60.0"
    )
endfunction()

add_subdirectory(metrics)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-metrics)
