project(userver-samples-grpc_middleware_service CXX)

add_executable(${PROJECT_NAME} src/main.cpp
                               src/client/view.cpp
                               src/http_handlers/say-hello/view.cpp
                               src/service/view.cpp
                               src/middlewares/client/component.cpp
                               src/middlewares/client/middleware.cpp
                               src/middlewares/server/component.cpp
                               src/middlewares/server/middleware.cpp
                               src/middlewares/auth.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE src)

# /// [gRPC middleware sample - CMake]
include(GrpcTargets)
userver_add_grpc_library(${PROJECT_NAME}-proto PROTOS samples/greeter.proto)
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}-proto)
# /// [gRPC middleware sample - CMake]

userver_sample_testsuite_add()
