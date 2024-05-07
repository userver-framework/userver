project(userver-ydb-tests CXX)

add_custom_target(${PROJECT_NAME})

add_subdirectory(basic)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-basic)
