project(userver-grpc-tests-metrics CXX)

add_executable(${PROJECT_NAME} src/main.cpp)

include(GrpcTargets)
userver_add_grpc_library(${PROJECT_NAME}-proto PROTOS samples/greeter.proto)
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}-proto)

userver_chaos_testsuite_add(
  TESTS_DIRECTORY tests
)
