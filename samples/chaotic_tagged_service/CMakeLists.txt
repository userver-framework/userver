cmake_minimum_required(VERSION 3.14)
project(userver-samples-chaotic-tagged_service CXX)

find_package(
    userver
    COMPONENTS core chaotic chaotic-openapi
    REQUIRED
)

# /// [schemas cmake]
# The types of every document are generated once, into the namespace and path it declares, by its own target.
# The types of the paths document are shared by its client and handlers.
userver_target_generate_chaotic(
    ${PROJECT_NAME}-schemas_objs
    SCHEMAS "${CMAKE_CURRENT_SOURCE_DIR}/schemas/errors.yaml" "${CMAKE_CURRENT_SOURCE_DIR}/schemas/greetings.yaml"
            "${CMAKE_CURRENT_SOURCE_DIR}/api/greeter.yaml"
)
# /// [schemas cmake]

# /// [api cmake]
# The handlers and the client generated from one document include the schema headers instead of copying them
userver_target_generate_openapi_handlers(
    ${PROJECT_NAME}-handlers_objs
    NAME greeter
    SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src"
    SCHEMAS "${CMAKE_CURRENT_SOURCE_DIR}/api/greeter.yaml"
)
userver_target_generate_openapi_client(
    ${PROJECT_NAME}-client_objs
    NAME upstream
    SCHEMAS "${CMAKE_CURRENT_SOURCE_DIR}/api/greeter.yaml"
)
target_link_libraries(${PROJECT_NAME}-client_objs ${PROJECT_NAME}-schemas_objs)
target_link_libraries(${PROJECT_NAME}-handlers_objs ${PROJECT_NAME}-schemas_objs ${PROJECT_NAME}-client_objs)
# /// [api cmake]

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}-handlers_objs)

userver_generate_config_yaml(
    ${PROJECT_NAME}
    BASE_CONFIGS "${CMAKE_CURRENT_SOURCE_DIR}/static_config.yaml"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/config.yaml"
)

userver_testsuite_add_simple(CONFIG_PATH "${CMAKE_CURRENT_BINARY_DIR}/config.yaml")
