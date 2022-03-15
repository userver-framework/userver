project(userver-samples CXX)

add_custom_target(${PROJECT_NAME})

add_subdirectory(config_service)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-config_service)

add_subdirectory(testsuite-support)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-testsuite-support)

# TODO: detect flatbuf support with --filename-suffix
if (NOT OPEN_SOURCE_BUILD)
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

if (USERVER_FEATURE_MONGODB AND USERVER_FEATURE_POSTGRESQL AND USERVER_FEATURE_REDIS AND USERVER_FEATURE_GRPC)
    # TODO remove the usage of taxi-specific utilities
    find_program(PYTEST NAMES taxi-pytest3 pytest)
    if(NOT PYTEST)
      message(FATAL_ERROR "pytest is not installed properly")
    endif()

    add_test(
      NAME ${PROJECT_NAME}-test
      COMMAND ${CMAKE_BINARY_DIR}/testsuite/env
          --databases postgresql,redis,mongo --log-level debug
          run -- ${PYTEST} ${CMAKE_CURRENT_SOURCE_DIR}/tests/
          --build-dir=${CMAKE_BINARY_DIR}
    )
    set_tests_properties(${PROJECT_NAME}-test PROPERTIES TIMEOUT 30)
endif()
