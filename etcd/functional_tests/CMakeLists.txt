project(userver-etcd-tests CXX)

add_custom_target(${PROJECT_NAME})

add_subdirectory(client)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-client)
