cmake_minimum_required(VERSION 3.1)
project(PackageTest CXX)

find_package(userver REQUIRED)
find_package(Python COMPONENTS Interpreter Development)

add_library(${PROJECT_NAME}_objs OBJECT
    hello.hpp
    hello.cpp
)
target_link_libraries(${PROJECT_NAME}_objs PRIVATE userver::core)

add_executable(${PROJECT_NAME}_core test_core.cpp)
target_link_libraries(${PROJECT_NAME}_core ${PROJECT_NAME}_objs userver::core)

add_executable(${PROJECT_NAME}_universal test_universal.cpp)
target_link_libraries(${PROJECT_NAME}_universal ${PROJECT_NAME}_objs userver::universal)

if (TARGET userver::utest)
    add_executable(${PROJECT_NAME}_utest test_utest.cpp)
    target_link_libraries(${PROJECT_NAME}_utest ${PROJECT_NAME}_objs userver::utest)
    add_google_tests(${PROJECT_NAME}_utest)
endif()

if (TARGET userver::ubench)
    add_executable(${PROJECT_NAME}_ubench test_ubench.cpp)
    target_link_libraries(${PROJECT_NAME}_ubench ${PROJECT_NAME}_objs userver::ubench)
    add_google_benchmark_tests(${PROJECT_NAME}_ubench)
endif()

if (TARGET userver::postgresql)
    add_executable(${PROJECT_NAME}_postgresql test_postgresql.cpp)
    target_link_libraries(${PROJECT_NAME}_postgresql ${PROJECT_NAME}_objs userver::postgresql)
endif()

if (TARGET userver::redis)
    add_executable(${PROJECT_NAME}_redis test_redis.cpp)
    target_link_libraries(${PROJECT_NAME}_redis ${PROJECT_NAME}_objs userver::redis)
endif()

if (TARGET userver::rabbitmq)
    add_executable(${PROJECT_NAME}_rabbitmq test_rabbitmq.cpp)
    target_link_libraries(${PROJECT_NAME}_rabbitmq ${PROJECT_NAME}_objs userver::rabbitmq)
endif()

if (TARGET userver::grpc)
    add_executable(${PROJECT_NAME}_grpc test_grpc.cpp)
    userver_add_grpc_library(${PROJECT_NAME}_proto PROTOS greeter.proto)
    target_link_libraries(${PROJECT_NAME}_grpc ${PROJECT_NAME}_objs userver::grpc ${PROJECT_NAME}_proto)
endif()

if (TARGET userver::mongo)
    add_executable(${PROJECT_NAME}_mongo test_mongo.cpp)
    target_link_libraries(${PROJECT_NAME}_mongo ${PROJECT_NAME}_objs userver::mongo)
endif()

if (TARGET userver::clickhouse)
    add_executable(${PROJECT_NAME}_clickhouse test_clickhouse.cpp)
    target_link_libraries(${PROJECT_NAME}_clickhouse ${PROJECT_NAME}_objs userver::clickhouse)
endif()

if (TARGET userver::kafka)
    add_executable(${PROJECT_NAME}_kafka test_kafka.cpp)
    target_link_libraries(${PROJECT_NAME}_kafka ${PROJECT_NAME}_objs userver::kafka)
endif()

add_subdirectory(hello_service)
