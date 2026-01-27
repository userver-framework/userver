project(userver-redis-tests CXX)

add_custom_target(${PROJECT_NAME})

function(userver_chaos_testsuite_add_redis)
    userver_chaos_testsuite_add(
        ENV REDIS_SLEEP_WORKAROUND_SECONDS=120
    )
endfunction()

add_subdirectory(basic_chaos)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-basic-chaos)

add_subdirectory(database_index)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-database-index)

add_subdirectory(metrics)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-metrics)

add_subdirectory(pubsub)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-pubsub)

add_subdirectory(integration_tests)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-integration)
