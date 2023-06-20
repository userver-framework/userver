project(userver-grpc-tests-metrics CXX)

add_executable(${PROJECT_NAME} service.cpp)

include(GrpcTargets)
add_grpc_library(${PROJECT_NAME}_proto PROTOS samples/greeter.proto)
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}_proto)

userver_chaos_testsuite_add(
  TESTS_DIRECTORY tests
)
