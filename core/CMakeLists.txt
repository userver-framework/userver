project(userver-core CXX)

include(GetUserverVersion)
message(STATUS "Userver version ${USERVER_VERSION}")

set(USERVER_NAMESPACE "userver" CACHE STRING "C++ namespace to use")
if (NOT "${USERVER_NAMESPACE}" STREQUAL "")
    set(USERVER_NAMESPACE_BEGIN "namespace ${USERVER_NAMESPACE} {" CACHE STRING "Open C++ namespace to use")
    set(USERVER_NAMESPACE_END "}" CACHE STRING "Close C++ namespace to use")
endif()

include(RequireDWCAS)
include(CheckFunctionExists)
check_function_exists("accept4" HAVE_ACCEPT4)
check_function_exists("pipe2" HAVE_PIPE2)

set(BUILD_CONFIG ${CMAKE_CURRENT_BINARY_DIR}/build_config.hpp)
if(${CMAKE_SOURCE_DIR}/.git/HEAD IS_NEWER_THAN ${BUILD_CONFIG})
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/build_config.hpp.in
    ${BUILD_CONFIG}
    ESCAPE_QUOTES @ONLY)
endif()

file(GLOB_RECURSE SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/../shared/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../shared/*.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
)

file(GLOB_RECURSE UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/../shared/*_test.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/*_test.cpp
)

file(GLOB_RECURSE LIBUTEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/testing/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/testing/*.hpp
)
list(REMOVE_ITEM LIBUTEST_SOURCES ${UNIT_TEST_SOURCES})

list(REMOVE_ITEM SOURCES ${UNIT_TEST_SOURCES} ${LIBUTEST_SOURCES})

file(GLOB_RECURSE BENCH_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/../shared/*_benchmark.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/*_benchmark.cpp
)
file(GLOB_RECURSE LIBUBENCH_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/core_benchmark.cpp
)
list (REMOVE_ITEM SOURCES ${BENCH_SOURCES} ${LIBUBENCH_SOURCES})

file(GLOB_RECURSE INTERNAL_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/internal/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/internal/*.hpp
)

list (REMOVE_ITEM SOURCES ${INTERNAL_SOURCES})

set(CMAKE_THREAD_PREFER_PTHREAD ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_package(Boost REQUIRED COMPONENTS
    program_options
    filesystem
    locale
    regex
    iostreams
)
find_package_required(LibEv "libev-dev")
find_package_required(ZLIB "zlib1g-dev")

if (USERVER_FEATURE_UTEST)
    include(SetupGTest)
    include(SetupGBench)
endif()
include(SetupSpdlog)

option(USERVER_FEATURE_SPDLOG_TCP_SINK "Use tcp_sink.h of the spdlog library for testing logs" ON)
if (NOT USERVER_FEATURE_SPDLOG_TCP_SINK)
  set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logging/component.cpp
    PROPERTIES COMPILE_FLAGS -DUSERVER_FEATURE_NO_SPDLOG_TCP_SINK=1
  )
endif()

include(SetupCAres)
include(SetupCURL)
include(SetupCryptoPP)

find_package(Iconv REQUIRED)
find_package_required(libyamlcpp "libyaml-cpp-dev")
find_package_required(OpenSSL "libssl-dev")
include(SetupFmt)
include(SetupCCTZ)

find_package_required(Http_Parser "libhttp-parser-dev")

userver_add_library(${PROJECT_NAME} SOURCES ${SOURCES})
target_compile_definitions(${PROJECT_NAME} PRIVATE SPDLOG_PREVENT_CHILD_FD SPDLOG_FMT_EXTERNAL)

# https://github.com/jemalloc/jemalloc/issues/820
if (USERVER_FEATURE_JEMALLOC AND NOT USERVER_SANITIZE AND NOT MACOS)
  if (USERVER_OPEN_SOURCE_BUILD)
      find_package_required(Jemalloc "libjemalloc-dev")
  else()
      find_package_required(Jemalloc "libyandex-taxi-jemalloc-dev")
  endif()
  target_link_libraries(${PROJECT_NAME} PUBLIC Jemalloc)
  target_compile_definitions(${PROJECT_NAME} PRIVATE JEMALLOC_ENABLED)
endif()

# https://bugs.llvm.org/show_bug.cgi?id=16404
if (USERVER_SANITIZE AND NOT CMAKE_BUILD_TYPE MATCHES "^Rel")
  target_link_libraries(${PROJECT_NAME} PUBLIC userver-compiler-rt-parts)
endif()

userver_target_link_libraries(${PROJECT_NAME}
  PUBLIC
    Threads::Threads
    Boost::locale
    sanitize-target
    libyamlcpp
    fmt
    c-ares
    cctz
    CURL::libcurl
    userver-stacktrace
  PRIVATE
    userver-uboost-coro
    Boost::filesystem
    Boost::program_options
    Boost::iostreams
    Boost::regex
    CryptoPP
    Http_Parser
    Iconv::Iconv
    LibEv
    OpenSSL::Crypto
    OpenSSL::SSL
    ZLIB::ZLIB
    spdlog_header_only
)

if (NOT MACOS)
  target_link_libraries(${PROJECT_NAME} PUBLIC atomic)
endif()

set(USERVER_PUBLIC_INCLUDE_DIRS
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/../shared/include
  ${USERVER_THIRD_PARTY_DIRS}/pfr/include
)
set(USERVER_GLIBC_INCLUDE_FIXES_DIR
  ${CMAKE_CURRENT_SOURCE_DIR}/libc_include_fixes
)

target_include_directories(${PROJECT_NAME} PUBLIC
  ${USERVER_PUBLIC_INCLUDE_DIRS}
)
target_include_directories(${PROJECT_NAME} SYSTEM BEFORE PUBLIC
  ${USERVER_GLIBC_INCLUDE_FIXES_DIR}
)

# The bug is only triggered with optimizations enabled -- TAXICOMMON-1729
set_property(
  SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/engine/errno_test.cpp
  APPEND PROPERTY COMPILE_FLAGS -O2
)

target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE
  ${USERVER_THIRD_PARTY_DIRS}/rapidjson/include
)

target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC
  ${USERVER_THIRD_PARTY_DIRS}/moodycamel/include
)

target_include_directories(${PROJECT_NAME}
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../shared/src/
    ${CMAKE_CURRENT_SOURCE_DIR}/src/
    ${CMAKE_CURRENT_BINARY_DIR}
)

file(TO_NATIVE_PATH "${CMAKE_SOURCE_DIR}/" SRC_LOG_PATH_BASE)
file(TO_NATIVE_PATH "${CMAKE_BINARY_DIR}/" BIN_LOG_PATH_BASE)
target_compile_definitions(${PROJECT_NAME} PUBLIC
  USERVER_LOG_SOURCE_PATH_BASE=${SRC_LOG_PATH_BASE}
  USERVER_LOG_BUILD_PATH_BASE=${BIN_LOG_PATH_BASE}
)

message(STATUS "Putting userver into namespace '${USERVER_NAMESPACE}': ${USERVER_NAMESPACE_BEGIN} ${USERVER_NAMESPACE_END}")
target_compile_definitions(${PROJECT_NAME} PUBLIC
  "USERVER_NAMESPACE=${USERVER_NAMESPACE}"
  "USERVER_NAMESPACE_BEGIN=${USERVER_NAMESPACE_BEGIN}"
  "USERVER_NAMESPACE_END=${USERVER_NAMESPACE_END}"
)

userver_add_library(userver-core-internal SOURCES ${INTERNAL_SOURCES})
target_compile_definitions(userver-core-internal PUBLIC $<TARGET_PROPERTY:${PROJECT_NAME},COMPILE_DEFINITIONS>)
target_include_directories(userver-core-internal PUBLIC
  $<TARGET_PROPERTY:${PROJECT_NAME},INCLUDE_DIRECTORIES>
  ${CMAKE_CURRENT_SOURCE_DIR}/internal/include
)
target_link_libraries(userver-core-internal
  PUBLIC
    ${PROJECT_NAME}
)

if (USERVER_FEATURE_UTEST)
    userver_add_library(userver-utest SOURCES ${LIBUTEST_SOURCES})
    target_compile_definitions(userver-utest PUBLIC $<TARGET_PROPERTY:${PROJECT_NAME},COMPILE_DEFINITIONS>)

    userver_target_link_libraries(userver-utest
      PUBLIC
        libgtest
        libgmock
        userver-core-internal
        ${PROJECT_NAME}
    PRIVATE
        Boost::program_options
    )
    target_include_directories(userver-utest PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/testing/include
    )
    target_include_directories(userver-utest PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/
        ${CMAKE_CURRENT_SOURCE_DIR}/testing/src/
    )

    # Target with no need to use userver namespace, but includes require userver/
    add_library(yandex-userver-utest INTERFACE)
    target_link_libraries(yandex-userver-utest INTERFACE userver-utest)
endif()

if (USERVER_IS_THE_ROOT_PROJECT)
    add_executable(${PROJECT_NAME}_unittest ${UNIT_TEST_SOURCES})
    target_include_directories (${PROJECT_NAME}_unittest PRIVATE
        $<TARGET_PROPERTY:${PROJECT_NAME},INCLUDE_DIRECTORIES>
    )
    target_link_libraries(${PROJECT_NAME}_unittest
      userver-utest
    )

    # We keep testing deprecated functions, no need to warn about that
    target_compile_options(${PROJECT_NAME}_unittest PRIVATE "-Wno-deprecated-declarations")
    add_google_tests(${PROJECT_NAME}_unittest)
endif()

if (USERVER_FEATURE_UTEST)
  userver_add_library(userver-ubench SOURCES ${LIBUBENCH_SOURCES})
  target_include_directories(userver-ubench PUBLIC $<TARGET_PROPERTY:${PROJECT_NAME},INCLUDE_DIRECTORIES>)
  target_compile_definitions(userver-ubench PUBLIC $<TARGET_PROPERTY:${PROJECT_NAME},COMPILE_DEFINITIONS>)
  userver_target_link_libraries(userver-ubench
    PUBLIC
      libbenchmark
      userver-core-internal
        ${PROJECT_NAME}
    )

# Target with no need to use userver namespace, but includes require userver/
add_library(yandex-userver-ubench INTERFACE)
target_link_libraries(yandex-userver-ubench INTERFACE userver-ubench)
endif()

if (USERVER_IS_THE_ROOT_PROJECT)
    add_executable(${PROJECT_NAME}_benchmark ${BENCH_SOURCES})
    target_link_libraries(${PROJECT_NAME}_benchmark PUBLIC userver-ubench)
    add_google_benchmark_tests(${PROJECT_NAME}_benchmark)
endif()

# Target with no need to use userver namespace, but includes require userver/
add_library(yandex-${PROJECT_NAME} INTERFACE)
target_link_libraries(yandex-${PROJECT_NAME} INTERFACE ${PROJECT_NAME})

userver_install(TARGETS ${PROJECT_NAME})
