project(userver-samples CXX)

add_custom_target(${PROJECT_NAME})

function(userver_sample_testsuite_add)
    # /// [testsuite - cmake]
    userver_testsuite_add(
      NAME testsuite-${PROJECT_NAME}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests
      PYTHON_BINARY ${TESTSUITE_PYTHON_BINARY}
      PYTEST_ARGS
      --service-config=${CMAKE_CURRENT_SOURCE_DIR}/static_config.yaml
      --service-source-dir=${CMAKE_CURRENT_SOURCE_DIR}
      --service-binary=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}
    )
    # /// [testsuite - cmake]
endfunction()

add_subdirectory(config_service)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-config_service)

add_subdirectory(testsuite-support)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-testsuite-support)

# TODO: detect flatbuf support with --filename-suffix
if (NOT USERVER_OPEN_SOURCE_BUILD)
    add_subdirectory(flatbuf_service)
    add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-flatbuf_service)
endif()

if (USERVER_FEATURE_GRPC)
    add_subdirectory(grpc_service)
    add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-grpc_service)
endif()

add_subdirectory(hello_service)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-hello_service)

if (USERVER_FEATURE_MONGODB)
    add_subdirectory(http_caching)
    add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-http_caching)

    add_subdirectory(mongo_service)
    add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-mongo_service)
endif()

if (USERVER_FEATURE_POSTGRESQL)
    add_subdirectory(postgres_service)
    add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-postgres_service)
endif()

add_subdirectory(production_service)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-production_service)

if (USERVER_FEATURE_REDIS)
    add_subdirectory(redis_service)
    add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-redis_service)
endif()

if (USERVER_FEATURE_CLICKHOUSE)
    add_subdirectory(clickhouse_service)
    add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-clickhouse_service)
endif()
