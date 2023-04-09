project(userver-grpc CXX)

include(GrpcTargets)

add_library(${PROJECT_NAME}-deps INTERFACE)
target_include_directories(${PROJECT_NAME}-deps INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
if (USERVER_CONAN)
    target_link_libraries(${PROJECT_NAME}-deps INTERFACE
        protobuf::protobuf grpc::grpc
    )
    if ("${GRPC_LIBRARY_VERSION}" VERSION_GREATER_EQUAL "1.41")
        target_link_libraries(${PROJECT_NAME}-deps INTERFACE
            absl::base absl::synchronization
        )
    endif()
else()
	target_link_libraries(${PROJECT_NAME}-deps INTERFACE Protobuf Grpc)
endif()


include(SetupGoogleProtoApis)

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
        tests/same_service_and_method_name.proto
      INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/proto
    )
endif()

file(GLOB_RECURSE UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*_test.cpp
)
list(REMOVE_ITEM SOURCES ${UNIT_TEST_SOURCES})

if (api-common-proto_USRV_SOURCES)
  list(APPEND SOURCES ${api-common-proto_USRV_SOURCES})
endif()

add_library(${PROJECT_NAME} STATIC ${SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if (DEFINED api-common-proto_LIBRARY)
  target_link_libraries(${PROJECT_NAME} PUBLIC ${api-common-proto_LIBRARY})
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC userver-core)

set(CHANNELZ_MIN_VERSION "1.17.0")
if (${GRPC_LIBRARY_VERSION} VERSION_GREATER_EQUAL ${CHANNELZ_MIN_VERSION})
    set(HAS_CHANNELZ TRUE)
else()
    set(HAS_CHANNELZ FALSE)
endif()
option(USERVER_FEATURE_GRPC_CHANNELZ "Enable Channelz for gRPC" ${HAS_CHANNELZ})
if (${USERVER_FEATURE_GRPC_CHANNELZ} AND NOT ${HAS_CHANNELZ})
    message(FATAL_ERROR
        "For Channelz, install gRPC >= ${CHANNELZ_MIN_VERSION}"
        "(found: ${GRPC_LIBRARY_VERSION})")
endif()

if (USERVER_FEATURE_GRPC_CHANNELZ)
    message(STATUS "gRPC Channelz enabled")
    if (USERVER_CONAN)
        target_link_libraries(${PROJECT_NAME} PUBLIC gRPC::grpcpp_channelz)
    else()
        find_package(GrpcChannelz REQUIRED)
        target_link_libraries(${PROJECT_NAME} PUBLIC GrpcChannelz)
    endif()
else()
    message(STATUS "gRPC Channelz disabled, install gRPC >= ${CHANNELZ_MIN_VERSION} to enable")
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        "USERVER_DISABLE_GRPC_CHANNELZ=1"
    )
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC ${PROJECT_NAME}-deps)

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
