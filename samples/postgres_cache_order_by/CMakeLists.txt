project(userver-samples-postgres_cache_order_by CXX)

add_executable(${PROJECT_NAME} "main.cpp")
target_link_libraries(${PROJECT_NAME} userver::postgresql)

userver_testsuite_add_simple()
