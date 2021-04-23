cmake_minimum_required(VERSION 3.12)

project(yandex-taxi-userver-core)

include(GetUserverVersion)
message(STATUS "Userver version ${USERVER_VERSION}")

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

list(REMOVE_ITEM SOURCES ${UNIT_TEST_SOURCES} ${LIBUTEST_SOURCES})

file(GLOB_RECURSE BENCH_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/../shared/*_benchmark.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/*_benchmark.cpp
)
file(GLOB_RECURSE LIBUBENCH_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/core_benchmark.cpp
)
list (REMOVE_ITEM SOURCES ${BENCH_SOURCES} ${LIBUBENCH_SOURCES})

set(CMAKE_THREAD_PREFER_PTHREAD ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_package(Boost REQUIRED COMPONENTS
    program_options
    filesystem
    locale
    thread
    regex
    iostreams
)
find_package_required(LibEv "libev-dev")
find_package_required(ZLIB "zlib1g-dev")
find_package(Helpergtest REQUIRED)
find_package(Helpergbench REQUIRED)
find_package(Helperspdlog REQUIRED)
find_package_required_version(CurlYandex
  "libyandex-taxi-curl4-openssl-dev (>= 7.68.0-10)"
  7.68.0)
find_package_required(CryptoPP "libcrypto++-dev")
find_package(Iconv REQUIRED)
find_package_required(libyamlcpp "libyaml-cpp-dev")
find_package_required(OpenSSL "libssl-dev")
find_package_required(Fmt "libfmt-dev (>= 7.1.3)")
find_package_required(cctz "libcctz-dev (>= 2.3-1+taxi1)")

find_package_required_version(Http_Parser
    "libhttp-parser-dev" 2.8)

add_library(${PROJECT_NAME} STATIC ${SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

# https://github.com/jemalloc/jemalloc/issues/820
if(NOT SANITIZE)
  find_package_required(Jemalloc "libyandex-taxi-jemalloc-dev")
  target_link_libraries(${PROJECT_NAME} PUBLIC Jemalloc)
  # disabled in https://st.yandex-team.ru/TAXICOMMON-1623
  if (NOT MACOS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE JEMALLOC_ENABLED)
  endif(NOT MACOS)
endif(NOT SANITIZE)

# https://bugs.llvm.org/show_bug.cgi?id=16404
if(SANITIZE AND NOT CMAKE_BUILD_TYPE MATCHES "^Rel")
  target_link_libraries(${PROJECT_NAME} PUBLIC yandex-taxi-compiler-rt-parts)
endif(SANITIZE AND NOT CMAKE_BUILD_TYPE MATCHES "^Rel")

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    Threads::Threads
    Boost::locale
    sanitize-target
    libyamlcpp
    Fmt
    cctz::cctz
    CurlYandex
    yandex-taxi-stacktrace
  PRIVATE
    yandex-taxi-userver-uboost-coro
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
    spdlog
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
set(
  USERVER_STANDALONE_ENGINE_INCLUDE_DIR
  ${CMAKE_CURRENT_SOURCE_DIR}/standalone_engine_include
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
  ${USERVER_THIRD_PARTY_DIRS}/moodycamel/include
)

target_include_directories(${PROJECT_NAME}
  PRIVATE
    ${USERVER_STANDALONE_ENGINE_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../shared/src/
    ${CMAKE_CURRENT_SOURCE_DIR}/src/
    ${CMAKE_CURRENT_BINARY_DIR}
)

set(LIBSTANDALONE_ENGINE yandex-taxi-userver+standalone_engine)
add_library(${LIBSTANDALONE_ENGINE} INTERFACE)
target_link_libraries(${LIBSTANDALONE_ENGINE} INTERFACE ${PROJECT_NAME}
    yandex-taxi-userver-uboost-coro)
target_include_directories(${LIBSTANDALONE_ENGINE} INTERFACE
  ${USERVER_STANDALONE_ENGINE_INCLUDE_DIR}
)

file(TO_NATIVE_PATH "${CMAKE_SOURCE_DIR}/" SRC_LOG_PATH_BASE)
file(TO_NATIVE_PATH "${CMAKE_BINARY_DIR}/" BIN_LOG_PATH_BASE)
target_compile_definitions(${PROJECT_NAME} PUBLIC
  USERVER_LOG_SOURCE_PATH_BASE=${SRC_LOG_PATH_BASE}
  USERVER_LOG_BUILD_PATH_BASE=${BIN_LOG_PATH_BASE}
)

add_library(utest STATIC ${LIBUTEST_SOURCES})
set_target_properties(utest PROPERTIES LINKER_LANGUAGE CXX)
target_compile_definitions(utest PUBLIC $<TARGET_PROPERTY:${PROJECT_NAME},COMPILE_DEFINITIONS>)

target_link_libraries(utest
  PUBLIC
    libgtest
    libgmock
    ${PROJECT_NAME}
  PRIVATE
    Boost::program_options
    ${LIBSTANDALONE_ENGINE}
)
target_include_directories(utest PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/testing/include
)
target_include_directories(utest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/
)

add_executable(${PROJECT_NAME}_unittest ${UNIT_TEST_SOURCES})
target_include_directories (${PROJECT_NAME}_unittest SYSTEM PRIVATE
    $<TARGET_PROPERTY:${PROJECT_NAME},INCLUDE_DIRECTORIES>
)
target_link_libraries(${PROJECT_NAME}_unittest
  utest
)
add_google_tests(${PROJECT_NAME}_unittest)


add_library(ubench ${LIBUBENCH_SOURCES})
target_include_directories(ubench SYSTEM PUBLIC $<TARGET_PROPERTY:${PROJECT_NAME},INCLUDE_DIRECTORIES>)
target_compile_definitions(ubench PUBLIC $<TARGET_PROPERTY:${PROJECT_NAME},COMPILE_DEFINITIONS>)
target_link_libraries(ubench
  PUBLIC
    libbenchmark
    ${PROJECT_NAME}
  PRIVATE
    ${LIBSTANDALONE_ENGINE}
)

add_executable(${PROJECT_NAME}_benchmark ${BENCH_SOURCES})
set_target_properties(${PROJECT_NAME}_benchmark PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(${PROJECT_NAME}_benchmark PUBLIC ubench)
