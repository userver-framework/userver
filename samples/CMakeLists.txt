project(userver-samples CXX)

add_custom_target(${PROJECT_NAME})

function(userver_sample_testsuite_add)
    set(PYTEST_ARGS_CONFIG_VARS "")

    set(CONFIG_VARS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/config_vars.yaml")
    if (EXISTS "${CONFIG_VARS_PATH}")
        set(PYTEST_ARGS_CONFIG_VARS ${PYTEST_ARGS_CONFIG_VARS} "--service-config-vars=${CONFIG_VARS_PATH}")
    endif()

    set(SECDIST_PATH "${CMAKE_CURRENT_SOURCE_DIR}/secure_data.json")
    if (EXISTS "${SECDIST_PATH}")
        set(PYTEST_ARGS_CONFIG_VARS ${PYTEST_ARGS_CONFIG_VARS} "--service-secdist=${SECDIST_PATH}")
    endif()

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

add_subdirectory(config_service)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-config_service)

add_subdirectory(testsuite-support)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-testsuite-support)

if (USERVER_FEATURE_GRPC)
    add_subdirectory(grpc_service)
    add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-grpc_service)
endif()

add_subdirectory(hello_service)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-hello_service)

add_subdirectory(tcp_service)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-tcp_service)

add_subdirectory(tcp_full_duplex_service)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-tcp_full_duplex_service)

if (USERVER_FEATURE_MONGODB)
    add_subdirectory(http_caching)
    add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-http_caching)

    add_subdirectory(mongo_service)
    add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-mongo_service)

    add_subdirectory(mongo-support)
    add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-mongo-support)
endif()

if (USERVER_FEATURE_POSTGRESQL)
    add_subdirectory(postgres_service)
    add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-postgres_service)

    add_subdirectory(postgres_auth)
    add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-postgres_auth)

    add_subdirectory(postgres-support)
    add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-postgres-support)
endif()

add_subdirectory(production_service)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-production_service)

add_subdirectory(static_service)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-static_service)

if (USERVER_FEATURE_REDIS)
    add_subdirectory(redis_service)
    add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-redis_service)
endif()

if (USERVER_FEATURE_CLICKHOUSE)
    add_subdirectory(clickhouse_service)
    add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-clickhouse_service)
endif()

if (USERVER_FEATURE_RABBITMQ)
    add_subdirectory(rabbitmq_service)
    add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-rabbitmq_service)
endif()

if (USERVER_FEATURE_MYSQL)
    add_subdirectory(mysql_service)
    add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-mysql_service)
endif()
