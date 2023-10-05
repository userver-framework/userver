project(userver-core-tests-tracing CXX)

add_executable(${PROJECT_NAME} "echo_no_body.cpp")
target_link_libraries(${PROJECT_NAME} userver-core)

userver_chaos_testsuite_add()
