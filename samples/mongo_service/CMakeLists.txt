cmake_minimum_required(VERSION 3.14)
project(userver-samples-mongo_service CXX)

find_package(userver COMPONENTS mongo REQUIRED)

add_executable(${PROJECT_NAME} "mongo_service.cpp")
target_link_libraries(${PROJECT_NAME} userver::mongo)

add_executable(${PROJECT_NAME}-unittest "unittests/mongo_test.cpp")
target_link_libraries(${PROJECT_NAME}-unittest userver::utest userver::mongo)

userver_add_utest(NAME ${PROJECT_NAME}-unittest DATABASES mongo)

userver_testsuite_add_simple()
