cmake_minimum_required(VERSION 3.14)
project(userver-samples-kafka_service CXX)

find_package(userver COMPONENTS core kafka REQUIRED)

add_library(${PROJECT_NAME}_objs OBJECT
    src/consume.hpp
    src/consume.cpp
    src/produce.hpp
    src/produce.cpp
    src/consumer_handler.hpp
    src/consumer_handler.cpp
    src/producer_handler.hpp
    src/producer_handler.cpp
)
target_link_libraries(${PROJECT_NAME}_objs userver::core userver::kafka)
target_include_directories(${PROJECT_NAME}_objs PUBLIC src/)

add_executable(${PROJECT_NAME} "main.cpp")
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}_objs)

userver_testsuite_add_simple(
    TEST_ENV
    "TESTSUITE_KAFKA_SERVER_START_TIMEOUT=120.0"
    "TESTSUITE_KAFKA_SERVER_HOST=[::1]"
    "TESTSUITE_KAFKA_CUSTOM_TOPICS=test-topic-1:1,test-topic-2:1"
)
