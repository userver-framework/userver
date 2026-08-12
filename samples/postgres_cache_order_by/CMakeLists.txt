cmake_minimum_required(VERSION 3.14)
project(userver-samples-postgres_cache_order_by CXX)

find_package(
    userver
    COMPONENTS postgresql
    REQUIRED
)

add_executable(${PROJECT_NAME} "main.cpp")
target_link_libraries(${PROJECT_NAME} userver::postgresql)

userver_testsuite_add_simple()
