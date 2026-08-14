project(userver-grpc-tests-light-client CXX)

add_executable(${PROJECT_NAME} main.cpp src/blocking_retry_limiter.cpp src/non_blocking_retry_limiter.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE src)

userver_add_grpc_library(${PROJECT_NAME}-proto PROTOS samples/greeter.proto)
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}-proto)

userver_chaos_testsuite_add(TESTS_DIRECTORY tests)
