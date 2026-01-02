project(userver-grpc-tests-error-status CXX)

file(GLOB SOURCES src/*.*pp)

add_executable(${PROJECT_NAME} main.cpp ${SOURCES})

include(UserverGrpcTargets)
userver_add_grpc_library(${PROJECT_NAME}-proto PROTOS functional_tests/error_status.proto)
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}-proto)

target_include_directories(${PROJECT_NAME} PRIVATE src)

userver_chaos_testsuite_add(TESTS_DIRECTORY tests)
