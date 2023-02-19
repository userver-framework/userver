cmake_minimum_required(VERSION 3.1)
project(PackageTest CXX)

find_package(userver REQUIRED)

add_library(${PROJECT_NAME}_objs OBJECT
    hello.hpp
    hello.cpp
)
target_link_libraries(${PROJECT_NAME}_objs PUBLIC userver-core)

add_executable(${PROJECT_NAME}_core test_core.cpp)

target_link_libraries(${PROJECT_NAME}_core ${PROJECT_NAME}_objs userver-utest)

add_executable(${PROJECT_NAME}_utest test_utest.cpp)

target_link_libraries(${PROJECT_NAME}_utest ${PROJECT_NAME}_objs userver-utest)
add_google_tests(${PROJECT_NAME}_utest)

add_executable(${PROJECT_NAME}_ubench test_ubench.cpp)

target_link_libraries(${PROJECT_NAME}_ubench ${PROJECT_NAME}_objs userver-ubench)
add_google_benchmark_tests(${PROJECT_NAME}_ubench)
