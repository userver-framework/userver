project(userver-grpc CXX)

if (USERVER_OPEN_SOURCE_BUILD)
    find_package(UserverGrpc REQUIRED)
    find_package(UserverProtobuf REQUIRED)
    add_library(Grpc ALIAS UserverGrpc)  # Unify link names
    add_library(Protobuf ALIAS UserverProtobuf)  # Unify link names
else()
    find_package(Grpc REQUIRED)
    find_package(Protobuf REQUIRED)
endif()
include(GrpcTargets)

file(GLOB_RECURSE SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)

add_grpc_library(${PROJECT_NAME}_unittest_proto
  PROTOS
    # Absolute paths are allowed
    ${CMAKE_CURRENT_SOURCE_DIR}/proto/tests/unit_test.proto
    # As well as paths relative to CMAKE_CURRENT_SOURCE_DIR
    tests/messages.proto
  INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/proto
)

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

if (USERVER_IS_THE_ROOT_PROJECT)
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
endif()

# Target with no need to use userver namespace, but includes require userver/
add_library(yandex-${PROJECT_NAME} INTERFACE)
target_link_libraries(yandex-${PROJECT_NAME} INTERFACE ${PROJECT_NAME})
