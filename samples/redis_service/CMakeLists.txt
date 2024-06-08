cmake_minimum_required(VERSION 3.14)
project(userver-samples-redis_service CXX)

find_package(userver COMPONENTS redis REQUIRED)

add_executable(${PROJECT_NAME} "redis_service.cpp")
target_link_libraries(${PROJECT_NAME} userver::redis)

userver_testsuite_add_simple()
