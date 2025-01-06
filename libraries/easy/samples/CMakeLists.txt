project(userver-easy-samples CXX)

add_custom_target(${PROJECT_NAME})

# Imitate an installed userver in samples.
file(WRITE "${CMAKE_BINARY_DIR}/package_stubs/userverConfig.cmake")

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

add_subdirectory(5_pg_service_template)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-pg-service-template)

add_subdirectory(6_pg_service_template_no_http_with)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-pg-service-template-no-http-with)

# Clean up fake userver installation above to avoid messing
# with find_package(userver) in the parent project.
file(REMOVE "${CMAKE_BINARY_DIR}/package_stubs/userverConfig.cmake")
