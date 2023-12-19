project(userver-postgresql-tests CXX)

add_custom_target(${PROJECT_NAME})

add_subdirectory(basic_chaos)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-basic-chaos)

add_subdirectory(connlimit_max)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-connlimit-max)

add_subdirectory(metrics)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-metrics)
