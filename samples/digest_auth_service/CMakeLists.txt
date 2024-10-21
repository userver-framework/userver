cmake_minimum_required(VERSION 3.14)
project(userver-samples-digest_auth CXX)

find_package(
  userver
  COMPONENTS postgresql
  REQUIRED)

add_executable(${PROJECT_NAME} digest_auth_service.cpp auth_digest.cpp)
target_link_libraries(${PROJECT_NAME} userver::postgresql)

userver_testsuite_add_simple()

set_tests_properties(
  testsuite-${PROJECT_NAME}
  PROPERTIES
    ENVIRONMENT
    "SERVER_DIGEST_AUTH_SECRET={\"http_server_digest_auth_secret\": \"some-private-key\"}"
)
