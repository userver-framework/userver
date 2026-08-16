cmake_minimum_required(VERSION 3.14)
project(userver-samples-chaotic-openapi_auth_service CXX)

find_package(
    userver
    COMPONENTS core chaotic
    REQUIRED
)

add_library(${PROJECT_NAME}_objs OBJECT src/auth_bearer.cpp)
target_link_libraries(${PROJECT_NAME}_objs userver::core)
target_include_directories(${PROJECT_NAME}_objs PUBLIC src)

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}_objs)

# /// [chaotic-handler] cmake
userver_target_generate_openapi_handlers(
    ${PROJECT_NAME}-handler-secure_objs
    NAME secure
    OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/handlers/secure"
    SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src"
    SCHEMAS "${CMAKE_CURRENT_SOURCE_DIR}/handlers/secure/openapi.yaml"
)
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}-handler-secure_objs)
# /// [chaotic-handler]

# /// [generate-config]
userver_generate_config_yaml(
    ${PROJECT_NAME}
    BASE_CONFIGS "${CMAKE_CURRENT_SOURCE_DIR}/static_config.yaml"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/config.yaml"
)
# /// [generate-config]

userver_testsuite_add_simple(CONFIG_PATH "${CMAKE_CURRENT_BINARY_DIR}/config.yaml")