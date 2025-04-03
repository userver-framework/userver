cmake_minimum_required(VERSION 3.14)
project(userver-easy-samples-json CXX)

find_package(userver COMPONENTS easy chaotic REQUIRED)

file(GLOB_RECURSE SCHEMAS ${CMAKE_CURRENT_SOURCE_DIR}/schemas/*.yaml)
userver_target_generate_chaotic(${PROJECT_NAME}-chgen
    ARGS
        # Map '/components/schemas/*' JSONSchema types to C++ types in 'schemas' namespace
        -n "/components/schemas/([^/]*)/=schemas::{0}"
        # Generate serializers for responses
        --generate-serializers
    OUTPUT_DIR
        ${CMAKE_CURRENT_BINARY_DIR}/src
    SCHEMAS
        ${SCHEMAS}
    RELATIVE_TO
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(${PROJECT_NAME} "main.cpp")
target_link_libraries(${PROJECT_NAME} userver::easy ${PROJECT_NAME}-chgen)

userver_testsuite_add_simple(DUMP_CONFIG True)
