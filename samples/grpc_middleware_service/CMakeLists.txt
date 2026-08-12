cmake_minimum_required(VERSION 3.14)
project(userver-samples-grpc_middleware_service CXX)

find_package(
    userver
    COMPONENTS grpc
    REQUIRED
)

add_executable(
    ${PROJECT_NAME}
    main.cpp
    src/client/view.cpp
    src/http_handlers/say-hello/view.cpp
    src/service/view.cpp
    src/middlewares/client/auth.cpp
    src/middlewares/client/chaos.cpp
    src/middlewares/server/auth.cpp
    src/middlewares/server/meta_filter.cpp
    src/middlewares/auth.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE src)

# /// [gRPC middleware sample - CMake]
userver_add_grpc_library(${PROJECT_NAME}-proto PROTOS samples/greeter.proto)
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}-proto)
# /// [gRPC middleware sample - CMake]

userver_testsuite_add_simple()
