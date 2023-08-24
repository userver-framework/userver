project(userver-postgresql-tests-metrics CXX)

add_executable(${PROJECT_NAME} "service.cpp")
target_link_libraries(${PROJECT_NAME} userver-postgresql)

userver_chaos_testsuite_add()
