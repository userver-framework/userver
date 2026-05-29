cmake_minimum_required(VERSION 3.14)
project(userver-samples-chaotic-openapi_service CXX)

find_package(
    userver
    COMPONENTS core chaotic
    REQUIRED
)

add_library(${PROJECT_NAME}_objs OBJECT src/say_hello.hpp src/say_hello.cpp src/hello_handler.hpp src/hello_handler.cpp)
target_link_libraries(${PROJECT_NAME}_objs userver::core)
target_include_directories(${PROJECT_NAME}_objs PUBLIC src)

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}_objs)

# /// [chaotic] cmake
userver_target_generate_openapi_client(
    ${PROJECT_NAME}-client-test_objs
    NAME test
    OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/clients/test"
    SCHEMAS ${CMAKE_CURRENT_SOURCE_DIR}/clients/test.yaml
    ARGS "--dynamic-config" "TEST_CLIENT_QOS"
)
target_link_libraries(${PROJECT_NAME}_objs ${PROJECT_NAME}-client-test_objs)
# /// [chaotic]

# /// [chaotic-handler] cmake
userver_target_generate_openapi_handlers(
    ${PROJECT_NAME}-handler-insecure_objs
    NAME insecure
    OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/handlers/insecure"
    SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src"
    SCHEMAS "${CMAKE_CURRENT_SOURCE_DIR}/handlers/insecure/openapi.yaml"
)
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}-handler-insecure_objs)
# /// [chaotic-handler]

# /// [generate-config]
userver_generate_config_yaml(
    ${PROJECT_NAME}
    BASE_CONFIGS "${CMAKE_CURRENT_SOURCE_DIR}/static_config.user.yaml"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/config.yaml"
)
# /// [generate-config]

userver_testsuite_add_simple(
    CONFIG_PATH "${CMAKE_CURRENT_BINARY_DIR}/config.yaml"
)
