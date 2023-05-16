project(userver-grpc-handlers-tests-basic-chaos CXX)

add_executable(${PROJECT_NAME} service.cpp)
target_link_libraries(${PROJECT_NAME} userver-grpc-handlers)

include(GrpcTargets)

userver_chaos_testsuite_add()
