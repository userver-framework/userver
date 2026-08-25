project(userver-core-tests-jemalloc CXX)

add_executable(${PROJECT_NAME} "main.cpp")
target_link_libraries(${PROJECT_NAME} userver::core)

userver_chaos_testsuite_add()
