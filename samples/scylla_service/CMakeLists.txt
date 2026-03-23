cmake_minimum_required(VERSION 3.14)
project(userver-samples-scylla_service CXX)

find_package(userver COMPONENTS scylla REQUIRED)
userver_setup_environment()

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} userver::scylla)
