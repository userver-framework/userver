cmake_minimum_required(VERSION 3.1)
project(PackageTest CXX)

find_package(userver REQUIRED)

include(GrpcTargets)
add_grpc_library(${PROJECT}_proto PROTOS greeter.proto)

add_executable(hello_service hello_service.cpp)
target_link_libraries(hello_service PRIVATE userver::userver ${PROJECT}_proto)