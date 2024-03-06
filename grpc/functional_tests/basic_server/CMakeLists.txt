project(userver-grpc-tests-basic-server CXX)

add_executable(${PROJECT_NAME} grpc_service.cpp)
target_link_libraries(${PROJECT_NAME} userver-core)

include(GrpcTargets)
userver_add_grpc_library(${PROJECT_NAME}-proto PROTOS samples/greeter.proto)
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}-proto)

userver_chaos_testsuite_add(
  TESTS_DIRECTORY tests-unix-socket
)
