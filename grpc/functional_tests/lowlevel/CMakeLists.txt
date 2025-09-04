project(userver-grpc-tests-lowlevel CXX)

add_executable(${PROJECT_NAME}
    main.cpp
    src/client_runner.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

include(UserverGrpcTargets)
userver_add_grpc_library(${PROJECT_NAME}-proto PROTOS samples/greeter.proto)
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}-proto)

userver_chaos_testsuite_add(TESTS_DIRECTORY tests-client-protocol)
