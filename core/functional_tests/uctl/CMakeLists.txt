project(userver-core-tests-uctl CXX)

add_executable(${PROJECT_NAME} "service.cpp")
target_link_libraries(${PROJECT_NAME} userver-core)

userver_chaos_testsuite_add()
