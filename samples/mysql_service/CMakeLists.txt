cmake_minimum_required(VERSION 3.14)
project(userver-samples-mysql_service CXX)

find_package(userver COMPONENTS mysql REQUIRED)
userver_setup_environment()

add_executable(${PROJECT_NAME} "mysql_service.cpp")
target_link_libraries(${PROJECT_NAME} userver::mysql)
target_link_libraries(${PROJECT_NAME} mariadbclient)

userver_testsuite_add_simple()
