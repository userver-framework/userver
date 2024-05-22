cmake_minimum_required(VERSION 3.14)
project(userver-samples-rabbitmq_service CXX)

if(NOT TARGET userver::rabbitmq)
  find_package(userver COMPONENTS rabbitmq REQUIRED)
  userver_setup_environment()
endif()

add_executable(${PROJECT_NAME} "rabbitmq_service.cpp")
target_link_libraries(${PROJECT_NAME} userver::rabbitmq)

userver_testsuite_add_simple(
    TEST_ENV "TESTSUITE_RABBITMQ_SERVER_START_TIMEOUT=120.0"
)
