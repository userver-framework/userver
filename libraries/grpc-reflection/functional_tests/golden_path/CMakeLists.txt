project(userver-grpc-reflection-tests-golden-path CXX)

add_executable(${PROJECT_NAME} "main.cpp")
target_link_libraries(${PROJECT_NAME} userver-core userver-grpc userver-grpc-reflection)

userver_chaos_testsuite_add()
