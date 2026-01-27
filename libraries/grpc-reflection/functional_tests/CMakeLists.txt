project(userver-grpc-reflection-tests CXX)

add_custom_target(${PROJECT_NAME})

add_subdirectory(golden_path)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-golden-path)
