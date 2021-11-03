cmake_minimum_required(VERSION 3.12)

project(userver-grpc CXX)

find_package(Grpc REQUIRED)
find_package(Protobuf REQUIRED)
include(GrpcTargets)

file(GLOB_RECURSE PROTOS ${CMAKE_CURRENT_SOURCE_DIR}/*.proto)

file(GLOB_RECURSE SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)

add_grpc_library(${PROJECT_NAME}_unittest_proto PROTOS ${PROTOS})

file(GLOB_RECURSE UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*_test.cpp
)
list(REMOVE_ITEM SOURCES ${UNIT_TEST_SOURCES})

add_library(${PROJECT_NAME} STATIC ${SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME} PUBLIC userver-core Grpc)

add_executable(${PROJECT_NAME}_unittest ${UNIT_TEST_SOURCES})
target_include_directories(${PROJECT_NAME}_unittest PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(${PROJECT_NAME}_unittest
  PUBLIC
    ${PROJECT_NAME}
    userver-utest
  PRIVATE
    ${PROJECT_NAME}_unittest_proto
)
add_google_tests(${PROJECT_NAME}_unittest)

# Target with no need to use userver namespace, but includes require userver/
add_library(yandex-${PROJECT_NAME} INTERFACE)
target_link_libraries(yandex-${PROJECT_NAME} INTERFACE ${PROJECT_NAME})
