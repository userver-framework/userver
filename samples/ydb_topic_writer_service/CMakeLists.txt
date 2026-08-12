cmake_minimum_required(VERSION 3.14)
project(userver-samples-ydb_topic_writer_service CXX)

find_package(
    userver
    COMPONENTS core ydb
    REQUIRED
)

add_executable(${PROJECT_NAME} main.cpp src/write_handler.cpp)
target_link_libraries(${PROJECT_NAME} userver::ydb)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src)

userver_testsuite_add_simple()
