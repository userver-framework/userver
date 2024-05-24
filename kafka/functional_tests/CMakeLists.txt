project(userver-kafka-tests CXX)

message(STATUS "Configuring Kafka functional_tests")

add_custom_target(${PROJECT_NAME})

add_subdirectory(balanced_consumer_groups)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-balanced-consumer-groups)

add_subdirectory(integrational_tests)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-integrational-tests)
