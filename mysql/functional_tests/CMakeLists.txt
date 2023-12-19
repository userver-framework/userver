project(userver-mysql-tests CXX)

add_custom_target(${PROJECT_NAME})

add_subdirectory(basic_chaos)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-basic-chaos)
