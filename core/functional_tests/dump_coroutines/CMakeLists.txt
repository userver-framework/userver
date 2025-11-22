project(userver-core-tests-dump-coroutines CXX)

add_executable(${PROJECT_NAME} "service.cpp")
target_link_libraries(${PROJECT_NAME} userver::core)

userver_chaos_testsuite_add()
