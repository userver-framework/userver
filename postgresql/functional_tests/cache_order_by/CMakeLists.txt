project(userver-postgresql-tests-cache-order-by CXX)

add_executable(${PROJECT_NAME} "service.cpp")
target_link_libraries(${PROJECT_NAME} userver-postgresql)

userver_chaos_testsuite_add()
