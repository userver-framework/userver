project(userver-samples-grpc_service CXX)

add_executable(${PROJECT_NAME} grpc_service.cpp)
target_link_libraries(${PROJECT_NAME} userver::core)

# /// [gRPC sample - CMake]
include(GrpcTargets)
userver_add_grpc_library(${PROJECT_NAME}-proto PROTOS samples/greeter.proto)
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}-proto)
# /// [gRPC sample - CMake]

userver_sample_testsuite_add()
