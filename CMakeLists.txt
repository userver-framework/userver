cmake_minimum_required(VERSION 3.14)
cmake_policy(SET CMP0025 NEW)
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.15")
  cmake_policy(SET CMP0093 NEW)
endif()
set(CMAKE_POLICY_DEFAULT_CMP0048 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0063 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)

project(userver)

set(USERVER_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}")

set(USERVER_NOT_INCLUDED_AS_SUBDIR OFF)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(USERVER_NOT_INCLUDED_AS_SUBDIR ON)
endif()

option(USERVER_FEATURE_CORE "Provide a core library with coroutines, otherwise build only userver-universal" ON)

set(USERVER_BUILD_PLATFORM_X86 OFF)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^x86")
  set(USERVER_BUILD_PLATFORM_X86 ${USERVER_FEATURE_CORE})
endif()

function(_require_userver_core FEATURE)
  if (NOT USERVER_FEATURE_CORE)
    message(FATAL_ERROR "'${FEATURE}' requires 'USERVER_FEATURE_CORE=ON'")
  endif()
endfunction()

option(USERVER_FEATURE_UTEST "Provide 'utest' and 'ubench' for unit testing and benchmarking coroutines" ${USERVER_FEATURE_CORE})
if (USERVER_FEATURE_UTEST)
  message(STATUS "Building utest with gtest and ubench with gbench")
endif()

option(
    USERVER_IS_THE_ROOT_PROJECT
    "Contributor mode: build userver tests, samples and helper tools"
    "${USERVER_NOT_INCLUDED_AS_SUBDIR}"
)
if (USERVER_IS_THE_ROOT_PROJECT)
  message(STATUS "Building userver as a primary project")
  if (NOT USERVER_FEATURE_UTEST)
    message(FATAL_ERROR "Cannot build tests without utest")
  endif()
else()
  message(STATUS "Building userver as a subproject")
endif()

set(USERVER_LIB_ENABLED_DEFAULT OFF)
if(USERVER_FEATURE_CORE AND USERVER_IS_THE_ROOT_PROJECT)
  set(USERVER_LIB_ENABLED_DEFAULT ON)
endif()

set(USERVER_MONGODB_DEFAULT OFF)
set(USERVER_CLICKHOUSE_DEFAULT OFF)
if(USERVER_FEATURE_CORE AND USERVER_IS_THE_ROOT_PROJECT AND USERVER_BUILD_PLATFORM_X86)
  if(NOT CMAKE_SYSTEM_NAME MATCHES "BSD")
    set(USERVER_MONGODB_DEFAULT ON)
  endif()
  set(USERVER_CLICKHOUSE_DEFAULT ON)
endif()

option(USERVER_CONAN "Build with Conan packages" ${CONAN_EXPORTED})

option(
    USERVER_DOWNLOAD_PACKAGES
    "Download missing third party packages and use the downloaded versions"
    ON
)
option(
    USERVER_FORCE_DOWNLOAD_PACKAGES
    "Download all possible third party packages even if a system package is available"
    OFF
)

option(USERVER_FEATURE_CRYPTOPP_BLAKE2 "Provide wrappers for blake2 algorithms of crypto++" ON)
if (NOT USERVER_FEATURE_CRYPTOPP_BLAKE2)
  add_compile_definitions("USERVER_NO_CRYPTOPP_BLAKE2=1")
endif()

option(USERVER_FEATURE_CRYPTOPP_BASE64_URL "Provide wrappers for Base64 URL decoding and encoding algorithms of crypto++" ON)
if (NOT USERVER_FEATURE_CRYPTOPP_BASE64_URL)
  add_compile_definitions("USERVER_NO_CRYPTOPP_BASE64_URL=1")
endif()

if(CMAKE_SYSTEM_NAME MATCHES "BSD")
  set(JEMALLOC_DEFAULT OFF)
else()
  set(JEMALLOC_DEFAULT ON)
endif()
option(USERVER_FEATURE_JEMALLOC "Enable linkage with jemalloc memory allocator" ${JEMALLOC_DEFAULT})

option(USERVER_DISABLE_PHDR_CACHE "Disable caching of dl_phdr_info items, which interferes with dlopen" OFF)

option(USERVER_CHECK_PACKAGE_VERSIONS "Check package versions" ON)

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  set(MACOS found)
endif()

include(cmake/UserverSetupEnvironment.cmake)
userver_setup_environment()

include(Sanitizers)
_userver_make_sanitize_target()

include(AddGoogleTests)
include(CheckSubmodule)
include(FindPackageRequired)
include(IncludeWhatYouUse)
# For userver_venv_setup below, otherwise FindPython would be enough.
include(UserverTestsuite)
include(CheckCompileFlags)

message(STATUS "Generating cmake files ...")
userver_venv_setup(
    NAME userver-cmake
    PYTHON_OUTPUT_VAR USERVER_CMAKE_GEN_PYTHON_BINARY
    REQUIREMENTS "${USERVER_ROOT_DIR}/scripts/external_deps/requirements.txt"
    UNIQUE
)
execute_process(
    COMMAND
    "${USERVER_CMAKE_GEN_PYTHON_BINARY}"
    -u "${USERVER_ROOT_DIR}/scripts/external_deps/cmake_generator.py"
    --repo-dir "${USERVER_ROOT_DIR}"
    --build-dir "${CMAKE_BINARY_DIR}"
    RESULT_VARIABLE RESULT
    WORKING_DIRECTORY "${USERVER_ROOT_DIR}"
)
if(RESULT)
  message(FATAL_ERROR
      "Generating cmake files failed with exit code: ${RESULT}"
  )
endif(RESULT)

set(USERVER_THIRD_PARTY_DIRS ${USERVER_ROOT_DIR}/third_party CACHE INTERNAL "")

init_debian_depends()

option(USERVER_FEATURE_MONGODB "Provide asynchronous driver for MongoDB" "${USERVER_MONGODB_DEFAULT}")
option(USERVER_FEATURE_POSTGRESQL "Provide asynchronous driver for PostgreSQL" "${USERVER_LIB_ENABLED_DEFAULT}")
option(USERVER_FEATURE_REDIS "Provide asynchronous driver for Redis" "${USERVER_LIB_ENABLED_DEFAULT}")
option(USERVER_FEATURE_GRPC "Provide asynchronous driver for gRPC" "${USERVER_LIB_ENABLED_DEFAULT}")
option(USERVER_FEATURE_CLICKHOUSE "Provide asynchronous driver for ClickHouse" "${USERVER_CLICKHOUSE_DEFAULT}")
option(USERVER_FEATURE_RABBITMQ "Provide asynchronous driver for RabbitMQ" "${USERVER_LIB_ENABLED_DEFAULT}")
option(USERVER_FEATURE_MYSQL "Provide asynchronous driver for MariaDB/MySQL" "${USERVER_LIB_ENABLED_DEFAULT}")

option(USERVER_FEATURE_UBOOST_CORO "Use vendored boost context instead of a system one" ON)

if (USERVER_FEATURE_GRPC)
  include(cmake/SetupProtobuf.cmake)
endif()

if (USERVER_IS_THE_ROOT_PROJECT)
  include(testsuite/SetupUserverTestsuiteEnv.cmake)
  add_subdirectory(testsuite)
endif()

add_subdirectory(universal "${CMAKE_BINARY_DIR}/userver/universal")

if (USERVER_FEATURE_CORE)
  add_subdirectory(core "${CMAKE_BINARY_DIR}/userver/core")
endif()

if (USERVER_IS_THE_ROOT_PROJECT AND USERVER_FEATURE_CORE)
  add_subdirectory(tools/engine)
  add_subdirectory(tools/httpclient)
  add_subdirectory(tools/netcat)
  add_subdirectory(tools/dns_resolver)
  add_subdirectory(tools/congestion_control_emulator)
endif()

if (USERVER_FEATURE_MONGODB)
  _require_userver_core("USERVER_FEATURE_MONGODB")
  add_subdirectory(mongo "${CMAKE_BINARY_DIR}/userver/mongo")
endif()

if (USERVER_FEATURE_POSTGRESQL)
  _require_userver_core("USERVER_FEATURE_POSTGRESQL")
  add_subdirectory(postgresql "${CMAKE_BINARY_DIR}/userver/postgresql")
endif()

if (USERVER_FEATURE_REDIS)
  _require_userver_core("USERVER_FEATURE_REDIS")
  add_subdirectory(redis "${CMAKE_BINARY_DIR}/userver/redis")
endif()

if (USERVER_FEATURE_GRPC)
  _require_userver_core("USERVER_FEATURE_GRPC")
  add_subdirectory(grpc "${CMAKE_BINARY_DIR}/userver/grpc")
endif()

if (USERVER_FEATURE_CLICKHOUSE)
  _require_userver_core("USERVER_FEATURE_CLICKHOUSE")
  add_subdirectory(clickhouse "${CMAKE_BINARY_DIR}/userver/clickhouse")
endif()

if (USERVER_FEATURE_RABBITMQ)
  _require_userver_core("USERVER_FEATURE_RABBITMQ")
  add_subdirectory(rabbitmq "${CMAKE_BINARY_DIR}/userver/rabbitmq")
endif()

if (USERVER_FEATURE_MYSQL)
  _require_userver_core("USERVER_FEATURE_MYSQL")
  add_subdirectory(mysql "${CMAKE_BINARY_DIR}/userver/mysql")
endif()

if (USERVER_IS_THE_ROOT_PROJECT AND USERVER_FEATURE_CORE)
  add_subdirectory(samples)
endif()
