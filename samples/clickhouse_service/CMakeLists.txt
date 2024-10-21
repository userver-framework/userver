cmake_minimum_required(VERSION 3.14)
project(userver-samples-clickhouse_service CXX)

find_package(
  userver
  COMPONENTS clickhouse
  REQUIRED)

add_executable(${PROJECT_NAME} "clickhouse_service.cpp")
target_link_libraries(${PROJECT_NAME} userver::clickhouse)

userver_testsuite_add_simple(TEST_ENV
                             "TESTSUITE_CLICKHOUSE_SERVER_START_TIMEOUT=120.0")
