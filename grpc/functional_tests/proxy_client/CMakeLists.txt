project(userver-grpc-tests-proxy-client CXX)

add_executable(${PROJECT_NAME} main.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE src)

userver_add_grpc_library(${PROJECT_NAME}-proto PROTOS samples/greeter.proto)
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}-proto)

userver_chaos_testsuite_add(TESTS_DIRECTORY tests-authority-proxy-available)
userver_chaos_testsuite_add(TESTS_DIRECTORY tests-authority-proxy-unavailable)
userver_chaos_testsuite_add(TESTS_DIRECTORY tests-no-proxy-target)
userver_chaos_testsuite_add(TESTS_DIRECTORY tests-proxy-disabled)
