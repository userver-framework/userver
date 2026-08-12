project(userver-sqlite-tests-metrics CXX)

add_executable(${PROJECT_NAME} "service.cpp")
target_link_libraries(${PROJECT_NAME} userver::sqlite)

userver_chaos_testsuite_add()
