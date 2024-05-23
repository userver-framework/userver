cmake_minimum_required(VERSION 3.14)
project(userver-samples-static_service CXX)

find_package(userver COMPONENTS core REQUIRED)
userver_setup_environment()

add_executable(${PROJECT_NAME} "static_service.cpp")
target_link_libraries(${PROJECT_NAME} userver::core)

userver_testsuite_add_simple()
