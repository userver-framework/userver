cmake_minimum_required(VERSION 3.14)
project(userver-samples-s3api CXX)

# /// [s3api] cmake
find_package(
    userver
    COMPONENTS core s3api
    REQUIRED
)

add_library(${PROJECT_NAME}_objs OBJECT src/s3api_client.cpp)
target_link_libraries(${PROJECT_NAME}_objs PUBLIC userver::s3api)
target_include_directories(${PROJECT_NAME}_objs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
# /// [s3api]

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}_objs)

# /// [gtest] cmake
add_executable(${PROJECT_NAME}-unittest unittests/client_test.cpp)
target_link_libraries(${PROJECT_NAME}-unittest ${PROJECT_NAME}_objs userver::utest userver::s3api-utest)
add_google_tests(${PROJECT_NAME}-unittest)
# /// [gtest]
