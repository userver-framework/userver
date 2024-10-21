project(userver-grpc CXX)

include(GrpcTargets)

add_library(${PROJECT_NAME}-deps INTERFACE)
target_include_directories(
  ${PROJECT_NAME}-deps
  INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(${PROJECT_NAME}-deps INTERFACE protobuf::libprotobuf
                                                     gRPC::grpc++)
if(USERVER_CONAN AND "${gRPC_VERSION}" VERSION_GREATER_EQUAL "1.41")
  target_link_libraries(${PROJECT_NAME}-deps INTERFACE absl::base
                                                       absl::synchronization)
endif()

if("${gRPC_VERSION}" VERSION_GREATER_EQUAL "1.66")
  message(
    FATAL_ERROR
      "userver currently does not support gRPC >= 1.66 due to a breaking "
      "change in grpc-core that requires a fix in userver "
      "https://github.com/grpc/grpc/pull/37321/files")
endif()

if(NOT TARGET userver-api-common-protos)
  include(SetupGoogleProtoApis)
endif()

file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/include/*pp
     ${CMAKE_CURRENT_SOURCE_DIR}/src/*pp)

file(GLOB_RECURSE UNIT_TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*_test.cpp
     ${CMAKE_CURRENT_SOURCE_DIR}/tests/*pp
     ${CMAKE_CURRENT_SOURCE_DIR}/utest/src/*_test.cpp)
list(REMOVE_ITEM SOURCES ${UNIT_TEST_SOURCES})

file(GLOB_RECURSE LIBUTEST_SOURCES
     ${CMAKE_CURRENT_SOURCE_DIR}/utest/include/*pp
     ${CMAKE_CURRENT_SOURCE_DIR}/utest/src/*pp)
list(REMOVE_ITEM LIBUTEST_SOURCES ${UNIT_TEST_SOURCES})
list(REMOVE_ITEM SOURCES ${LIBUTEST_SOURCES})

file(GLOB_RECURSE BENCH_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/*.cpp
     ${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/*.hpp)

if(api-common-proto_USRV_SOURCES)
  list(APPEND SOURCES ${api-common-proto_USRV_SOURCES})
endif()

add_library(${PROJECT_NAME}-internal STATIC ${SOURCES})

if(NOT USERVER_GPRC_BUILD_FROM_SOURCE)
  target_compile_definitions(${PROJECT_NAME}-internal
                             PUBLIC GRPC_ASAN_SUPPRESSED GRPC_TSAN_SUPPRESSED)
endif()

set_target_properties(${PROJECT_NAME}-internal PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(
  ${PROJECT_NAME}-internal
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

_userver_directory_install(
  COMPONENT grpc DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include DESTINATION
  ${CMAKE_INSTALL_INCLUDEDIR}/..)
if(DEFINED api-common-proto_LIBRARY)
  target_link_libraries(${PROJECT_NAME}-internal
                        PUBLIC ${api-common-proto_LIBRARY})
  _userver_install_targets(COMPONENT grpc TARGETS ${api-common-proto_LIBRARY})
endif()

userver_generate_grpc_files(
  PROTOS userver/field_options.proto GENERATED_INCLUDES proto_include_paths
  CPP_FILES generated_proto_sources)
add_library(${PROJECT_NAME}-proto STATIC ${generated_proto_sources})
target_link_libraries(${PROJECT_NAME}-proto PUBLIC ${PROJECT_NAME}-deps)
target_include_directories(${PROJECT_NAME}-proto SYSTEM
                           PUBLIC $<BUILD_INTERFACE:${proto_include_paths}>)
_userver_install_targets(COMPONENT grpc TARGETS ${PROJECT_NAME}-proto)

target_link_libraries(${PROJECT_NAME}-internal PUBLIC userver-core
                                                      ${PROJECT_NAME}-proto)

set(CHANNELZ_MIN_VERSION "1.17.0")
if(${gRPC_VERSION} VERSION_GREATER_EQUAL ${CHANNELZ_MIN_VERSION}
   AND NOT GRPC_USE_SYSTEM_PACKAGE)
  set(HAS_CHANNELZ TRUE)
else()
  set(HAS_CHANNELZ FALSE)
endif()
option(USERVER_FEATURE_GRPC_CHANNELZ "Enable Channelz for gRPC" ${HAS_CHANNELZ})
if(${USERVER_FEATURE_GRPC_CHANNELZ} AND NOT ${HAS_CHANNELZ})
  message(
    FATAL_ERROR
      "For Channelz, install gRPC >= ${CHANNELZ_MIN_VERSION}"
      "(found: ${gRPC_VERSION}); gRPC must be installed with CMake bindings")
endif()

if(USERVER_FEATURE_GRPC_CHANNELZ)
  message(STATUS "gRPC Channelz enabled")
  if(GRPC_USE_SYSTEM_PACKAGE)
    find_package(GrpcChannelz REQUIRED)
    if(NOT TARGET gRPC::grpcpp_channelz)
      add_library(gRPC::grpcpp_channelz ALIAS GrpcChannelz)
    endif()
  endif()
  target_link_libraries(${PROJECT_NAME}-internal PUBLIC gRPC::grpcpp_channelz)
else()
  message(
    STATUS
      "gRPC Channelz disabled, install gRPC >= ${CHANNELZ_MIN_VERSION} to enable"
  )
  target_compile_definitions(${PROJECT_NAME}-internal
                             PRIVATE "USERVER_DISABLE_GRPC_CHANNELZ=1")
endif()

target_link_libraries(${PROJECT_NAME}-internal PUBLIC ${PROJECT_NAME}-deps)
_userver_install_targets(COMPONENT grpc TARGETS ${PROJECT_NAME}-internal
                         ${PROJECT_NAME}-deps)

if(Protobuf_VERSION VERSION_GREATER_EQUAL 4.23.0)
  target_link_libraries(
    ${PROJECT_NAME}-internal
    PUBLIC absl::log_globals absl::log_internal_globals
           absl::log_internal_check_op)
endif()

if(USERVER_FEATURE_UTEST)
  add_library(${PROJECT_NAME}-utest STATIC ${LIBUTEST_SOURCES})

  target_link_libraries(${PROJECT_NAME}-utest PUBLIC ${PROJECT_NAME}-internal
                                                     userver::utest)

  target_include_directories(
    ${PROJECT_NAME}-utest
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/utest/include>)
  target_include_directories(
    ${PROJECT_NAME}-utest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/
                                  ${CMAKE_CURRENT_SOURCE_DIR}/utest/src/)
  _userver_install_targets(COMPONENT grpc TARGETS ${PROJECT_NAME}-utest)
  _userver_directory_install(
    COMPONENT grpc DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/utest/include
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/..)
endif()

if(USERVER_IS_THE_ROOT_PROJECT)
  userver_add_grpc_library(
    ${PROJECT_NAME}-unittest-proto
    PROTOS
    # Absolute paths are allowed
    ${CMAKE_CURRENT_SOURCE_DIR}/proto/tests/unit_test.proto
    # As well as paths relative to CMAKE_CURRENT_SOURCE_DIR
    tests/messages.proto
    tests/protobuf.proto
    tests/same_service_and_method_name.proto
    tests/global_package.proto
    tests/repeating_word_in_package_name.proto
    tests/secret_fields.proto
    INCLUDE_DIRECTORIES
    ${CMAKE_CURRENT_SOURCE_DIR}/proto)

  add_executable(${PROJECT_NAME}-unittest ${UNIT_TEST_SOURCES})
  target_include_directories(
    ${PROJECT_NAME}-unittest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
                                     ${CMAKE_CURRENT_SOURCE_DIR}/tests)
  target_link_libraries(
    ${PROJECT_NAME}-unittest
    PUBLIC ${PROJECT_NAME}-internal ${PROJECT_NAME}-utest
    PRIVATE ${PROJECT_NAME}-unittest-proto)
  add_google_tests(${PROJECT_NAME}-unittest)

  add_executable(${PROJECT_NAME}-benchmark ${BENCH_SOURCES})
  target_link_libraries(${PROJECT_NAME}-benchmark ${PROJECT_NAME}-internal
                        userver-ubench ${PROJECT_NAME}-unittest-proto)
  target_include_directories(
    ${PROJECT_NAME}-benchmark
    PRIVATE $<TARGET_PROPERTY:${PROJECT_NAME}-internal,INCLUDE_DIRECTORIES>)
  add_google_benchmark_tests(${PROJECT_NAME}-benchmark)

  add_subdirectory(functional_tests)
endif()

add_subdirectory(handlers)

add_library(${PROJECT_NAME} INTERFACE)
target_link_libraries(${PROJECT_NAME} INTERFACE ${PROJECT_NAME}-handlers)
target_link_libraries(${PROJECT_NAME} INTERFACE ${PROJECT_NAME}-internal)

_userver_install_targets(COMPONENT grpc TARGETS ${PROJECT_NAME})

_userver_directory_install(
  COMPONENT
  grpc
  FILES
  "${USERVER_ROOT_DIR}/cmake/SetupProtobuf.cmake"
  "${USERVER_ROOT_DIR}/cmake/SetupGrpc.cmake"
  "${USERVER_ROOT_DIR}/cmake/GrpcTargets.cmake"
  "${USERVER_ROOT_DIR}/cmake/install/userver-grpc-config.cmake"
  "${USERVER_ROOT_DIR}/cmake/modules/FindUserverGrpc.cmake"
  DIRECTORY
  "${USERVER_ROOT_DIR}/scripts/grpc"
  DESTINATION
  ${CMAKE_INSTALL_LIBDIR}/cmake/userver)
