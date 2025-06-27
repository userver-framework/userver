cmake_minimum_required(VERSION 3.14)
project(userver-samples-sqlite_service CXX)

find_package(
    userver
    COMPONENTS sqlite
    REQUIRED
)

add_executable(${PROJECT_NAME} "sqlite_service.cpp")
target_link_libraries(${PROJECT_NAME} userver::sqlite)

userver_testsuite_add_simple()
