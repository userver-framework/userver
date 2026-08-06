cmake_minimum_required(VERSION 3.14)
project(userver-samples-benchmark_service CXX)

find_package(
    userver
    COMPONENTS core postgresql chaotic
    REQUIRED
)
find_package(ZLIB REQUIRED)

add_library(
    ${PROJECT_NAME}_objs OBJECT
    src/dataset_provider.hpp
    src/dataset_provider.cpp
    src/handlers/async_db/handler.hpp
    src/handlers/async_db/handler.cpp
    src/handlers/baseline11/handler.hpp
    src/handlers/baseline11/handler.cpp
    src/handlers/baseline2/handler.hpp
    src/handlers/baseline2/handler.cpp
    src/handlers/json/handler.hpp
    src/handlers/json/handler.cpp
    src/handlers/plaintext/handler.hpp
    src/handlers/plaintext/handler.cpp
    src/handlers/upload/handler.hpp
    src/handlers/upload/handler.cpp
    src/middlewares/compression.hpp
    src/middlewares/compression.cpp
    src/middlewares/compression_pipeline_builder.hpp
)
target_link_libraries(${PROJECT_NAME}_objs userver::core userver::postgresql ZLIB::ZLIB)
target_include_directories(${PROJECT_NAME}_objs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

file(GLOB_RECURSE SCHEMAS ${CMAKE_CURRENT_SOURCE_DIR}/schemas/*.yaml)
userver_target_generate_chaotic(
    ${PROJECT_NAME}-chgen
    GENERATE_SERIALIZERS
    LAYOUT "/components/schemas/([^/]*)/=userver_httparena::{0}"
    OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/src
    SCHEMAS ${SCHEMAS}
    RELATIVE_TO ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(${PROJECT_NAME}_objs ${PROJECT_NAME}-chgen)

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}_objs)

userver_testsuite_add_simple()
