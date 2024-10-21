project(userver-grpc-tests-middleware-server CXX)

file(GLOB SOURCES src/*.*pp)

add_executable(${PROJECT_NAME} ${SOURCES})

include(GrpcTargets)
userver_add_grpc_library(${PROJECT_NAME}-proto PROTOS samples/greeter.proto)
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}-proto)

userver_chaos_testsuite_add(TESTS_DIRECTORY tests)
