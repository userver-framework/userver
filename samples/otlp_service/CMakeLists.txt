cmake_minimum_required(VERSION 3.14)
project(userver-samples-otlp_service CXX)

find_package(userver COMPONENTS otlp REQUIRED)

add_executable(${PROJECT_NAME} "main.cpp")
target_link_libraries(${PROJECT_NAME} userver::otlp)

userver_testsuite_add_simple()
