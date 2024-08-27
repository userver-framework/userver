cmake_minimum_required(VERSION 3.14)
project(userver-samples-grpc_service CXX)

# /// [ugrpc]
find_package(userver COMPONENTS grpc REQUIRED)

add_library(${PROJECT_NAME}_objs OBJECT
    # Note: it's nonsense to have the same client and service in the same executable.
    # For test and demonstration purposes only.
    src/greeter_client.cpp
    src/greeter_service.cpp
)
target_link_libraries(${PROJECT_NAME}_objs PUBLIC userver::grpc)
target_include_directories(${PROJECT_NAME}_objs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
# /// [ugrpc]

# /// [add_grpc_library]
userver_add_grpc_library(${PROJECT_NAME}-proto PROTOS samples/greeter.proto)
target_link_libraries(${PROJECT_NAME}_objs PUBLIC ${PROJECT_NAME}-proto)
# /// [add_grpc_library]

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}_objs)

# /// [gtest]
add_executable(${PROJECT_NAME}-unittest unittests/greeter_service_test.cpp)
target_link_libraries(${PROJECT_NAME}-unittest
    ${PROJECT_NAME}_objs
    userver::utest
    userver::grpc-utest
)
add_google_tests(${PROJECT_NAME}-unittest)
# /// [gtest]

userver_testsuite_add_simple()
