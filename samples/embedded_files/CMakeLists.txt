cmake_minimum_required(VERSION 3.14)
project(userver-samples-embedded_files CXX)

find_package(
    userver
    COMPONENTS core
    REQUIRED
)

add_library(${PROJECT_NAME}_objs OBJECT src/say_hello.hpp src/say_hello.cpp src/hello_handler.hpp src/hello_handler.cpp)
target_link_libraries(${PROJECT_NAME}_objs userver::core)
target_include_directories(${PROJECT_NAME}_objs PUBLIC src)

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}_objs)

# /// [embedded]
userver_embed_file(${PROJECT_NAME}_config NAME static_config_yaml FILEPATH static_config.yaml)
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}_config)
# /// [embedded]

userver_testsuite_add_simple()
