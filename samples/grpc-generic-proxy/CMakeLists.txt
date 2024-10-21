cmake_minimum_required(VERSION 3.14)
project(userver-samples-grpc-generic-proxy CXX)

find_package(
  userver
  COMPONENTS grpc
  REQUIRED)

add_executable(${PROJECT_NAME} main.cpp src/proxy_service.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE src)
target_link_libraries(${PROJECT_NAME} PRIVATE userver::grpc)

# Actually unused in the service, only needed for testsuite tests. We could
# generate just the Python bindings, but this approach is currently not
# implemented in userver_add_grpc_library.
userver_add_grpc_library(${PROJECT_NAME}-proto PROTOS samples/greeter.proto)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-proto)

userver_testsuite_add_simple()
