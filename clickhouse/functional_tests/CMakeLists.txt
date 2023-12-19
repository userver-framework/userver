project(userver-clickhouse-tests CXX)

add_custom_target(${PROJECT_NAME})

add_subdirectory(basic_chaos)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-basic-chaos)

add_subdirectory(metrics)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-metrics)
