project(userver-samples-grpc_service CXX)

add_executable(${PROJECT_NAME} grpc_service.cpp)
target_link_libraries(${PROJECT_NAME} userver-core)

# /// [gRPC sample - CMake]
include(GrpcTargets)
add_grpc_library(${PROJECT_NAME}_proto PROTOS samples/greeter.proto)
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}_proto)
# /// [gRPC sample - CMake]

userver_sample_testsuite_add()
