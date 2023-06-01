project(userver-grpc CXX)

include(GrpcTargets)

add_library(${PROJECT_NAME}-deps INTERFACE)
target_include_directories(${PROJECT_NAME}-deps INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(${PROJECT_NAME}-deps INTERFACE
    protobuf::libprotobuf gRPC::grpc++
)
if(USERVER_CONAN AND "${gRPC_VERSION}" VERSION_GREATER_EQUAL "1.41")
  target_link_libraries(
      ${PROJECT_NAME}-deps INTERFACE
      absl::base absl::synchronization
  )
endif()


include(SetupGoogleProtoApis)

file(GLOB_RECURSE SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/include/*pp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*pp)

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
    add_subdirectory(functional_tests)
endif()

file(GLOB_RECURSE UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*_test.cpp
)
list(REMOVE_ITEM SOURCES ${UNIT_TEST_SOURCES})

if (api-common-proto_USRV_SOURCES)
  list(APPEND SOURCES ${api-common-proto_USRV_SOURCES})
endif()

add_library(${PROJECT_NAME}-internal STATIC ${SOURCES})

set_target_properties(${PROJECT_NAME}-internal PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(${PROJECT_NAME}-internal
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if (DEFINED api-common-proto_LIBRARY)
  target_link_libraries(${PROJECT_NAME}-internal PUBLIC ${api-common-proto_LIBRARY})
endif()

target_link_libraries(${PROJECT_NAME}-internal PUBLIC userver-core)

set(CHANNELZ_MIN_VERSION "1.17.0")
if (${gRPC_VERSION} VERSION_GREATER_EQUAL ${CHANNELZ_MIN_VERSION})
    set(HAS_CHANNELZ TRUE)
else()
    set(HAS_CHANNELZ FALSE)
endif()
option(USERVER_FEATURE_GRPC_CHANNELZ "Enable Channelz for gRPC" ${HAS_CHANNELZ})
if (${USERVER_FEATURE_GRPC_CHANNELZ} AND NOT ${HAS_CHANNELZ})
    message(FATAL_ERROR
        "For Channelz, install gRPC >= ${CHANNELZ_MIN_VERSION}"
        "(found: ${gRPC_VERSION})")
endif()

if (USERVER_FEATURE_GRPC_CHANNELZ)
    message(STATUS "gRPC Channelz enabled")
    if(GRPC_USE_SYSTEM_PACKAGE)
        find_package(GrpcChannelz REQUIRED)
        if(NOT TARGET gRPC::grpcpp_channelz)
            add_library(gRPC::grpcpp_channelz ALIAS GrpcChannelz)
        endif()
    endif()
    target_link_libraries(${PROJECT_NAME}-internal PUBLIC gRPC::grpcpp_channelz)
else()
    message(STATUS "gRPC Channelz disabled, install gRPC >= ${CHANNELZ_MIN_VERSION} to enable")
    target_compile_definitions(${PROJECT_NAME}-internal PRIVATE
        "USERVER_DISABLE_GRPC_CHANNELZ=1"
    )
endif()

target_link_libraries(${PROJECT_NAME}-internal PUBLIC ${PROJECT_NAME}-deps)

if (USERVER_IS_THE_ROOT_PROJECT)
    add_executable(${PROJECT_NAME}_unittest ${UNIT_TEST_SOURCES})
    target_include_directories(${PROJECT_NAME}_unittest PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/include
      ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    target_link_libraries(${PROJECT_NAME}_unittest
      PUBLIC
        ${PROJECT_NAME}-internal
        userver-utest
      PRIVATE
        ${PROJECT_NAME}_unittest_proto
    )
    add_google_tests(${PROJECT_NAME}_unittest)

    target_compile_definitions(${PROJECT_NAME}_unittest PRIVATE
      DEFAULT_DYNAMIC_CONFIG_FILENAME="${CMAKE_SOURCE_DIR}/grpc/tests/dynamic_config_fallback.json"
    )
endif()

add_subdirectory(handlers)

add_library(${PROJECT_NAME} INTERFACE)
target_link_libraries(${PROJECT_NAME} INTERFACE ${PROJECT_NAME}-handlers)
target_link_libraries(${PROJECT_NAME} INTERFACE ${PROJECT_NAME}-internal)
