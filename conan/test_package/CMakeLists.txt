cmake_minimum_required(VERSION 3.1)
project(PackageTest CXX)

find_package(userver REQUIRED)
find_package (Python COMPONENTS Interpreter Development)

add_library(${PROJECT_NAME}_objs OBJECT
    hello.hpp
    hello.cpp
)
target_link_libraries(${PROJECT_NAME}_objs PUBLIC userver-core)

add_executable(${PROJECT_NAME}_core test_core.cpp)

target_link_libraries(${PROJECT_NAME}_core ${PROJECT_NAME}_objs userver-utest)

add_executable(${PROJECT_NAME}_utest test_utest.cpp)

target_link_libraries(${PROJECT_NAME}_utest ${PROJECT_NAME}_objs userver-utest)
add_google_tests(${PROJECT_NAME}_utest)

add_executable(${PROJECT_NAME}_ubench test_ubench.cpp)

target_link_libraries(${PROJECT_NAME}_ubench ${PROJECT_NAME}_objs userver-ubench)
add_google_benchmark_tests(${PROJECT_NAME}_ubench)

add_executable(${PROJECT_NAME}_postgresql test_postgresql.cpp)

target_link_libraries(${PROJECT_NAME}_postgresql ${PROJECT_NAME}_objs userver-postgresql)

add_executable(${PROJECT_NAME}_redis test_redis.cpp)

target_link_libraries(${PROJECT_NAME}_redis ${PROJECT_NAME}_objs userver-redis)

add_executable(${PROJECT_NAME}_rabbitmq test_rabbitmq.cpp)

target_link_libraries(${PROJECT_NAME}_rabbitmq ${PROJECT_NAME}_objs userver-rabbitmq)

add_executable(${PROJECT_NAME}_grpc test_grpc.cpp)

add_grpc_library(${PROJECT_NAME}_proto PROTOS greeter.proto)

target_link_libraries(${PROJECT_NAME}_grpc ${PROJECT_NAME}_objs userver-grpc ${PROJECT_NAME}_proto)

add_executable(${PROJECT_NAME}_mongo test_mongo.cpp)

target_link_libraries(${PROJECT_NAME}_mongo ${PROJECT_NAME}_objs userver-mongo)

set(PYTHON_BINARY ${Python_EXECUTABLE})
set(TESTSUITE_PYTHON_BINARY ${Python_EXECUTABLE})
add_subdirectory(../../samples/testsuite-support ${CMAKE_CURRENT_BINARY_DIR}/testsuite-support)

add_subdirectory(../../tools/json2yaml ${CMAKE_CURRENT_BINARY_DIR}/json2yaml)
