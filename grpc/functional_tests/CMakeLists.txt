project(userver-grpc-tests CXX)

add_custom_target(${PROJECT_NAME})

add_subdirectory(basic_chaos)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-basic-chaos)

add_subdirectory(metrics)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-metrics)

add_subdirectory(basic_server)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-basic-server)

add_subdirectory(middleware_server)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-middleware-server)
