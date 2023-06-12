project(userver-grpc CXX)

option(USERVER_FEATURE_GRPC_CHANNELZ "Enable Channelz for gRPC" ${USERVER_OPEN_SOURCE_BUILD})
if (NOT USERVER_FEATURE_GRPC_CHANNELZ)
  set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ugrpc/server/server.cpp
    PROPERTIES COMPILE_FLAGS -DUSERVER_DISABLE_GRPC_CHANNELZ=1
  )
endif()

if(USERVER_OPEN_SOURCE_BUILD)
    find_package(UserverGrpc REQUIRED)
    find_package(UserverProtobuf REQUIRED)
    add_library(Grpc ALIAS UserverGrpc)  # Unify link names
    add_library(Protobuf ALIAS UserverProtobuf)  # Unify link names

    if (USERVER_FEATURE_GRPC_CHANNELZ)
        find_package(GrpcChannelz REQUIRED)
    endif()
else()
    find_package(Grpc REQUIRED)
    find_package(Protobuf REQUIRED)

    if (USERVER_FEATURE_GRPC_CHANNELZ)
        message(FATAL_ERROR "gRPC Channelz is not supported in Tier1 configuration")
    endif()
endif()

include(GrpcTargets)

file(GLOB_RECURSE SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)

if (USERVER_IS_THE_ROOT_PROJECT)
    add_grpc_library(${PROJECT_NAME}_unittest_proto
      PROTOS
        # Absolute paths are allowed
        ${CMAKE_CURRENT_SOURCE_DIR}/proto/tests/unit_test.proto
        # As well as paths relative to CMAKE_CURRENT_SOURCE_DIR
        tests/messages.proto
      INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/proto
    )
endif()

file(GLOB_RECURSE UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*_test.cpp
)
list(REMOVE_ITEM SOURCES ${UNIT_TEST_SOURCES})

userver_add_library(${PROJECT_NAME} SOURCES ${SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME} PUBLIC userver-core Grpc)
if (USERVER_FEATURE_GRPC_CHANNELZ)
    target_link_libraries(${PROJECT_NAME} PUBLIC GrpcChannelz)
endif()

if (USERVER_IS_THE_ROOT_PROJECT)
    add_executable(${PROJECT_NAME}_unittest ${UNIT_TEST_SOURCES})
    target_include_directories(${PROJECT_NAME}_unittest PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/include
      ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    userver_target_link_libraries(${PROJECT_NAME}_unittest
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

userver_install(TARGETS ${PROJECT_NAME})
