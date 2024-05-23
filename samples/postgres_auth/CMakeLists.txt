cmake_minimum_required(VERSION 3.14)
project(userver-samples-postgres_auth CXX)

find_package(userver COMPONENTS postgresql REQUIRED)
userver_setup_environment()

add_executable(${PROJECT_NAME} postgres_service.cpp auth_bearer.cpp)
target_link_libraries(${PROJECT_NAME} userver::postgresql)

userver_testsuite_add_simple()
