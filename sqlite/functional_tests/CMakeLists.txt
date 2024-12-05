project(userver-sqlite-tests CXX)

add_custom_target(${PROJECT_NAME})

add_subdirectory(integration_tests)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-basic)
