project(userver-easy-samples CXX)

add_custom_target(${PROJECT_NAME})

add_subdirectory(0_hello_world)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-hello-world)

add_subdirectory(1_hi)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-hi)

add_subdirectory(2_key_value)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-key-value)

add_subdirectory(3_json)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-json)

add_subdirectory(4_custom_dependency)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-custom-dependency)
