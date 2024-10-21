cmake_minimum_required(VERSION 3.14)
project(userver-samples-tcp_full_duplex_service CXX)

find_package(
  userver
  COMPONENTS core
  REQUIRED)

add_executable(${PROJECT_NAME} "tcp_full_duplex_service.cpp")
target_link_libraries(${PROJECT_NAME} userver::core)

userver_testsuite_add_simple()
