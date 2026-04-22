project(userver-grpc-handlers-tests-health CXX)

add_executable(${PROJECT_NAME} service.cpp)
target_link_libraries(${PROJECT_NAME} userver-grpc-handlers)

include(UserverGrpcTargets)

userver_chaos_testsuite_add(
    TESTS_DIRECTORY tests
    PYTHONPATH "${CMAKE_CURRENT_BINARY_DIR}/../../proto"
)
userver_chaos_testsuite_add(
    TESTS_DIRECTORY tests-graceful-shutdown
    PYTHONPATH "${CMAKE_CURRENT_BINARY_DIR}/../../proto"
)
