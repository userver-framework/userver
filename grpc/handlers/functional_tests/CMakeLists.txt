project(userver-grpc-handlers-tests CXX)

add_custom_target(${PROJECT_NAME})

add_subdirectory(health)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-health)
