project(userver-kafka-tests-integrational-tests CXX)

add_executable(${PROJECT_NAME} "kafka_service.cpp")
target_link_libraries(${PROJECT_NAME} userver-core userver-kafka)
