cmake_minimum_required(VERSION 3.14)
project(userver-samples-mongo-support CXX)

find_package(userver COMPONENTS mongo REQUIRED)

file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} userver::mongo)

# /// [testsuite - cmake]

userver_testsuite_add_simple()

# /// [testsuite - cmake]
