cmake_minimum_required(VERSION 3.12)
cmake_policy(SET CMP0025 NEW)
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.15")
  cmake_policy(SET CMP0093 NEW)
endif()

project(userver)

set(USERVER_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}")

set(USERVER_NOT_INCLUDED_AS_SUBDIR OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(USERVER_NOT_INCLUDED_AS_SUBDIR ON)
endif()

option(USERVER_FEATURE_UTEST "Provide 'utest' and 'ubench' for unit testing and benchmarking coroutines" ON)
if (USERVER_FEATURE_UTEST)
  message(STATUS "Building utest with gtest and ubench with gbench")
endif()


option(USERVER_IS_THE_ROOT_PROJECT "Build tests, samples and helper tools" ${USERVER_NOT_INCLUDED_AS_SUBDIR})
if (USERVER_IS_THE_ROOT_PROJECT)
  message(STATUS "Building userver as a primary project")
  if (NOT USERVER_FEATURE_UTEST)
      message(FATAL_ERROR "Cannot build tests without utest")
  endif()
else()
  message(STATUS "Building userver as a subproject")
endif()

set(USERVER_BUILD_PLATFORM_X86 OFF)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^x86")
  set(USERVER_BUILD_PLATFORM_X86 ON)
endif()

set(USERVER_MONGODB ${USERVER_BUILD_PLATFORM_X86})
if(NOT MAC AND CMAKE_SYSTEM_NAME MATCHES "BSD")
  set(USERVER_MONGODB OFF)
endif()

option(USERVER_CONAN "Build with Conan packages" ${CONAN_EXPORTED})

option(USERVER_DOWNLOAD_PACKAGES "Download missing third party packages and use the downloaded versions" ON)

option(USERVER_FEATURE_CRYPTOPP_BLAKE2 "Provide wrappers for blake2 algorithms of crypto++" ON)
if (NOT USERVER_FEATURE_CRYPTOPP_BLAKE2)
  add_definitions("-DUSERVER_NO_CRYPTOPP_BLAKE2=1")
endif()

option(USERVER_FEATURE_CRYPTOPP_BASE64_URL "Provide wrappers for Base64 URL decoding and encoding algorithms of crypto++" ON)
if (NOT USERVER_FEATURE_CRYPTOPP_BASE64_URL)
  add_definitions("-DUSERVER_NO_CRYPTOPP_BASE64_URL=1")
endif()

if(CMAKE_SYSTEM_NAME MATCHES "BSD")
  set(JEMALLOC_DEFAULT OFF)
else()
  set(JEMALLOC_DEFAULT ON)
endif()
option(USERVER_FEATURE_JEMALLOC "Enable linkage with jemalloc memory allocator" ${JEMALLOC_DEFAULT})

option(USERVER_DISABLE_PHDR_CACHE "Disable caching of dl_phdr_info items, which interferes with dlopen" OFF)

option(USERVER_CHECK_PACKAGE_VERSIONS "Check package versions" ON)

include(cmake/SetupEnvironment.cmake)
include(AddGoogleTests)
include(CheckSubmodule)
include(Sanitizers)
include(FindPackageRequired)
include(IncludeWhatYouUse)
include(FindPython)

message(STATUS "Generating cmake files ...")
execute_process(
  COMMAND ${PYTHON}
    -m pip install --disable-pip-version-check
    -r ${USERVER_ROOT_DIR}/scripts/external_deps/requirements.txt
  WORKING_DIRECTORY ${USERVER_ROOT_DIR}
)
execute_process(
    COMMAND
        ${PYTHON} -u ${USERVER_ROOT_DIR}/scripts/external_deps/cmake_generator.py
        --repo-dir=${USERVER_ROOT_DIR}
        --build-dir=${CMAKE_BINARY_DIR}
    RESULT_VARIABLE RESULT
    WORKING_DIRECTORY ${USERVER_ROOT_DIR}
)

if (RESULT)
    message(FATAL_ERROR
        "Generating cmake files failed with exit code: ${RESULT}"
    )
endif(RESULT)

set(USERVER_THIRD_PARTY_DIRS ${USERVER_ROOT_DIR}/third_party CACHE INTERNAL "")

init_debian_depends()

option(USERVER_FEATURE_MONGODB "Provide asynchronous driver for MongoDB" ${USERVER_MONGODB})
option(USERVER_FEATURE_POSTGRESQL "Provide asynchronous driver for PostgreSQL" ON)
option(USERVER_FEATURE_REDIS "Provide asynchronous driver for Redis" ON)
option(USERVER_FEATURE_GRPC "Provide asynchronous driver for gRPC" ON)
option(USERVER_FEATURE_CLICKHOUSE "Provide asynchronous driver for ClickHouse" ${USERVER_BUILD_PLATFORM_X86})
option(USERVER_FEATURE_RABBITMQ "Provide asynchronous driver for RabbitMQ" ON)
option(USERVER_FEATURE_MYSQL "Provide asynchronous driver for MariaDB/MySQL" OFF)
option(USERVER_FEATURE_UNIVERSAL "Provide a universal utilities library that does not use coroutines" ON)
option(USERVER_FEATURE_UBOOST_CORO "Use vendored boost context instead of a system one" ON)

if (USERVER_FEATURE_GRPC)
    include(cmake/SetupProtobuf.cmake)
endif()

if (USERVER_IS_THE_ROOT_PROJECT)
    include(testsuite/SetupUserverTestsuiteEnv.cmake)
    add_subdirectory(testsuite)
endif()

if (USERVER_CONAN)
    find_package(Boost COMPONENTS stacktrace REQUIRED)
else()
    add_subdirectory("${USERVER_THIRD_PARTY_DIRS}/boost_stacktrace")
endif()

if (USERVER_FEATURE_UBOOST_CORO)
    add_subdirectory(uboost_coro)
else()
    find_package(Boost REQUIRED COMPONENTS
        context
        coroutine
    )
endif()

add_subdirectory(core "${CMAKE_BINARY_DIR}/userver/core")
add_subdirectory("${USERVER_THIRD_PARTY_DIRS}/compiler-rt")

if (USERVER_IS_THE_ROOT_PROJECT)
    add_subdirectory(tools/engine)
    add_subdirectory(tools/json2yaml)
    add_subdirectory(tools/httpclient)
    add_subdirectory(tools/netcat)
    add_subdirectory(tools/dns_resolver)
    add_subdirectory(tools/congestion_control_emulator)
endif()

if (USERVER_FEATURE_MONGODB)
    add_subdirectory(mongo "${CMAKE_BINARY_DIR}/userver/mongo")
endif()

if (USERVER_FEATURE_POSTGRESQL)
    add_subdirectory(postgresql "${CMAKE_BINARY_DIR}/userver/postgresql")
endif()

if (USERVER_FEATURE_REDIS)
    add_subdirectory(redis "${CMAKE_BINARY_DIR}/userver/redis")
endif()

if (USERVER_FEATURE_GRPC)
    add_subdirectory(grpc "${CMAKE_BINARY_DIR}/userver/grpc")
endif()

if (USERVER_FEATURE_CLICKHOUSE)
    add_subdirectory(clickhouse "${CMAKE_BINARY_DIR}/userver/clickhouse")
endif()

if (USERVER_FEATURE_RABBITMQ)
    add_subdirectory(rabbitmq "${CMAKE_BINARY_DIR}/userver/rabbitmq")
endif()

if (USERVER_FEATURE_MYSQL)
    add_subdirectory(mysql "${CMAKE_BINARY_DIR}/userver/mysql")
endif()

if (USERVER_FEATURE_UNIVERSAL)
  add_subdirectory(universal "${CMAKE_BINARY_DIR}/userver/universal")
endif()

if (USERVER_IS_THE_ROOT_PROJECT)
  add_subdirectory(samples)
endif()
