project(userver-samples-digest_auth CXX)

add_executable(${PROJECT_NAME} digest_auth_service.cpp auth_digest.cpp)
target_link_libraries(${PROJECT_NAME} userver-core userver-postgresql)

function(userver_multitest_testsuite_add)
    set(options)
    set(oneValueArgs TESTS_DIRECTORY)
    set(multiValueArgs)
    cmake_parse_arguments(
            ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    if (NOT ARG_TESTS_DIRECTORY)
        set(ARG_TESTS_DIRECTORY common)
    endif()

    userver_testsuite_add(
      SERVICE_TARGET ${PROJECT_NAME}-${ARG_TESTS_DIRECTORY}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests/${ARG_TESTS_DIRECTORY}
      PYTHON_BINARY ${TESTSUITE_PYTHON_BINARY}
      PYTEST_ARGS
      --service-config=${CMAKE_CURRENT_SOURCE_DIR}/static_config.yaml
      --service-source-dir=${CMAKE_CURRENT_SOURCE_DIR}
      --service-binary=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}
      --config-fallback=${CMAKE_CURRENT_SOURCE_DIR}/dynamic_config_fallback.json
      --service-config-vars=${CMAKE_CURRENT_SOURCE_DIR}/config_vars.yaml
    )
endfunction()

userver_multitest_testsuite_add(TESTS_DIRECTORY common)
userver_multitest_testsuite_add(TESTS_DIRECTORY proxy)
