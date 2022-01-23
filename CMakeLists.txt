cmake_minimum_required(VERSION 3.12)
cmake_policy(SET CMP0025 NEW)
project(userver)

set(CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/generated
  ${CMAKE_CURRENT_BINARY_DIR}/cmake_generated
)

set(USERVER_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}")

option(OPEN_SOURCE_BUILD "Build without Yandex and MLU packages" OFF)
option(USERVER_FEATURE_DOWNLOAD_PACKAGES "Download missing third party packages and use the downloaded versions" ${OPEN_SOURCE_BUILD})

option(USERVER_FEATURE_CRYPTOPP_BLAKE2 "Provide wrappers for blake2 algorithms of crypto++" ON)
if (NOT USERVER_FEATURE_CRYPTOPP_BLAKE2)
  add_definitions("-DUSERVER_NO_CRYPTOPP_BLAKE2=1")
endif()

if (NOT OPEN_SOURCE_BUILD)
  include(UserverCodegen)
endif()

include(SetupEnvironment)
include(AddGoogleTests)
include(CheckSubmodule)
include(Sanitizers)
include(FindPackageRequired)
include(IncludeWhatYouUse)

set(USERVER_THIRD_PARTY_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/third_party CACHE INTERNAL "")

init_debian_depends()

add_subdirectory(core "${CMAKE_BINARY_DIR}/userver/core")
add_subdirectory("${USERVER_THIRD_PARTY_DIRS}/boost_stacktrace")
add_subdirectory("${USERVER_THIRD_PARTY_DIRS}/compiler-rt")
add_subdirectory(uboost_coro)

if (NOT USERVICES)
    add_subdirectory(testsuite)
    add_subdirectory(tools/engine)
    add_subdirectory(tools/json2yaml)
    add_subdirectory(tools/httpclient)
    add_subdirectory(tools/netcat)
    add_subdirectory(tools/dns_resolver)
    add_subdirectory(tools/congestion_control_emulator)

    option(USERVER_FEATURE_MONGODB "Provide asynchronous driver for MongoDB" ON)
    if (USERVER_FEATURE_MONGODB)
        add_subdirectory(mongo "${CMAKE_BINARY_DIR}/userver/mongo")
    endif()

    option(USERVER_FEATURE_POSTGRESQL "Provide asynchronous driver for PostgreSQL" ON)
    if (USERVER_FEATURE_POSTGRESQL)
        add_subdirectory(postgresql "${CMAKE_BINARY_DIR}/userver/postgresql")
    endif()

    option(USERVER_FEATURE_REDIS "Provide asynchronous driver for Redis" ON)
    if (USERVER_FEATURE_REDIS)
        add_subdirectory(redis "${CMAKE_BINARY_DIR}/userver/redis")
    endif()

    option(USERVER_FEATURE_GRPC "Provide asynchronous driver for gRPC" ON)
    if (USERVER_FEATURE_GRPC)
        add_subdirectory(grpc "${CMAKE_BINARY_DIR}/userver/grpc")
    endif()

    option(USERVER_FEATURE_UNIVERSAL "Provide a universal utilities library that does not use coroutines" ON)
    if (USERVER_FEATURE_UNIVERSAL)
      add_subdirectory(universal "${CMAKE_BINARY_DIR}/userver/universal")
    endif()

    if (USERVER_FEATURE_MONGODB AND USERVER_FEATURE_POSTGRESQL AND USERVER_FEATURE_REDIS AND USERVER_FEATURE_GRPC AND USERVER_FEATURE_UNIVERSAL)
        add_subdirectory(samples)
    endif()
endif()
