cmake_minimum_required(VERSION 3.14)
project(userver-samples-redis_service CXX)

find_package(
    userver
    COMPONENTS redis
    REQUIRED
)

add_executable(${PROJECT_NAME} "main.cpp")
target_link_libraries(${PROJECT_NAME} userver::redis)

add_executable(${PROJECT_NAME}-unittest "unittests/redis_test.cpp")
target_link_libraries(${PROJECT_NAME}-unittest userver::utest userver::redis)

userver_add_utest(NAME ${PROJECT_NAME}-unittest DATABASES redis)

userver_testsuite_add_simple()
