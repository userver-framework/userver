cmake_minimum_required(VERSION 3.14)
project(userver-samples-kafka_service CXX)

find_package(userver COMPONENTS kafka REQUIRED)

add_executable(${PROJECT_NAME} "kafka_service.cpp")
target_link_libraries(${PROJECT_NAME} userver-kafka)

userver_testsuite_add_simple(
    TEST_ENV
    "TESTSUITE_KAFKA_SERVER_START_TIMEOUT=120.0"
    "TESTSUITE_KAFKA_CUSTOM_TOPICS=test-topic-1:1,test-topic-2:1"
)
