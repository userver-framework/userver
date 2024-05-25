cmake_minimum_required(VERSION 3.14)
project(userver-samples-grpc_service CXX)

find_package(userver COMPONENTS grpc REQUIRED)

add_executable(${PROJECT_NAME} grpc_service.cpp)
target_link_libraries(${PROJECT_NAME} userver::grpc)

# /// [gRPC sample - CMake]
userver_add_grpc_library(${PROJECT_NAME}-proto PROTOS samples/greeter.proto)
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}-proto)
# /// [gRPC sample - CMake]

userver_testsuite_add_simple()
