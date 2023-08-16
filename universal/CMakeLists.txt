project(userver-universal CXX)

set(USERVER_NAMESPACE "userver" CACHE STRING "C++ namespace to use")
if (NOT "${USERVER_NAMESPACE}" STREQUAL "")
    set(USERVER_NAMESPACE_BEGIN "namespace ${USERVER_NAMESPACE} {" CACHE STRING "Open C++ namespace to use")
    set(USERVER_NAMESPACE_END "}" CACHE STRING "Close C++ namespace to use")
endif()

if (NOT USERVER_ROOT_DIR)
  message(FATAL_ERROR, "Include userver project rather than the userver/universal")
endif()

if (TARGET userver-core)
  message(FATAL_ERROR, "userver-core should be included after userver-universal")
endif()

file(GLOB_RECURSE SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
)

file(GLOB_RECURSE UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*_test.cpp
)
file(GLOB_RECURSE BENCH_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*_benchmark.cpp
)
file(GLOB_RECURSE INTERNAL_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/internal/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/internal/*.hpp
)
list(REMOVE_ITEM SOURCES ${UNIT_TEST_SOURCES} ${BENCH_SOURCES} ${INTERNAL_SOURCES})

set(CMAKE_THREAD_PREFER_PTHREAD ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_package(Boost REQUIRED COMPONENTS program_options filesystem regex)

find_package(Iconv REQUIRED)
find_package_required(OpenSSL "libssl-dev")

if (USERVER_CONAN)
  find_package(cryptopp REQUIRED)
  find_package(yaml-cpp REQUIRED)
  find_package(fmt REQUIRED)
  find_package(cctz REQUIRED)

  find_package(RapidJSON REQUIRED)
  target_compile_definitions(rapidjson INTERFACE RAPIDJSON_HAS_STDSTRING)
else()
  include(SetupCryptoPP)
  find_package_required(libyamlcpp "libyaml-cpp-dev")
  include(SetupFmt)
  include(SetupCCTZ)
endif()

add_library(${PROJECT_NAME} STATIC ${SOURCES})

# https://github.com/jemalloc/jemalloc/issues/820
if (USERVER_FEATURE_JEMALLOC AND NOT USERVER_SANITIZE AND NOT MACOS)
  if (USERVER_CONAN)
    find_package(jemalloc REQUIRED)
    target_link_libraries(${PROJECT_NAME} PUBLIC jemalloc::jemalloc)
  else()
    find_package_required(Jemalloc "libjemalloc-dev")
    target_link_libraries(${PROJECT_NAME} PUBLIC Jemalloc)
  endif()

  target_compile_definitions(${PROJECT_NAME} PRIVATE JEMALLOC_ENABLED)
endif()

get_filename_component(BASE_PREFIX "${CMAKE_SOURCE_DIR}/../" ABSOLUTE)
file(TO_NATIVE_PATH "${CMAKE_SOURCE_DIR}/" SRC_LOG_PATH_BASE)
file(TO_NATIVE_PATH "${CMAKE_BINARY_DIR}/" BIN_LOG_PATH_BASE)

message(STATUS "SRC_LOG_PATH_BASE=${SRC_LOG_PATH_BASE}")
message(STATUS "BIN_LOG_PATH_BASE=${BIN_LOG_PATH_BASE}")
message(STATUS "BASE_PREFIX=${BASE_PREFIX}")

check_cxx_compiler_flag("-fmacro-prefix-map=a=b" USERVER_COMPILER_HAS_MACRO_PREFIX_MAP)
if (USERVER_COMPILER_HAS_MACRO_PREFIX_MAP)
  target_compile_options(${PROJECT_NAME} PUBLIC
    -fmacro-prefix-map=${SRC_LOG_PATH_BASE}/=
    -fmacro-prefix-map=${BIN_LOG_PATH_BASE}/=
    -fmacro-prefix-map=${BASE_PREFIX}/=
  )
else()
  target_compile_definitions(${PROJECT_NAME} PUBLIC
    USERVER_LOG_SOURCE_PATH_BASE=${SRC_LOG_PATH_BASE}
    USERVER_LOG_BUILD_PATH_BASE=${BIN_LOG_PATH_BASE}
    USERVER_LOG_PREFIX_PATH_BASE=${BASE_PREFIX}
  )
endif()

message(STATUS "Putting userver into namespace '${USERVER_NAMESPACE}': ${USERVER_NAMESPACE_BEGIN} ${USERVER_NAMESPACE_END}")
target_compile_definitions(${PROJECT_NAME} PUBLIC
  "USERVER_NAMESPACE=${USERVER_NAMESPACE}"
  "USERVER_NAMESPACE_BEGIN=${USERVER_NAMESPACE_BEGIN}"
  "USERVER_NAMESPACE_END=${USERVER_NAMESPACE_END}"
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
  CRYPTOPP_ENABLE_NAMESPACE_WEAK=1
)

# Suppress OpenSSL 3 warnings: we still primarily support OpenSSL 1.1.x
target_compile_definitions(${PROJECT_NAME} PRIVATE OPENSSL_SUPPRESS_DEPRECATED=)

# https://bugs.llvm.org/show_bug.cgi?id=16404
if (USERVER_SANITIZE AND NOT CMAKE_BUILD_TYPE MATCHES "^Rel")
  add_subdirectory("${USERVER_THIRD_PARTY_DIRS}/compiler-rt" compiler_rt_build)
  target_link_libraries(${PROJECT_NAME} PUBLIC userver-compiler-rt-parts)
endif()

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    Threads::Threads
    sanitize-target
  PRIVATE
    Boost::filesystem
    Boost::program_options
    Boost::regex
    OpenSSL::Crypto
    OpenSSL::SSL
)

if (USERVER_CONAN)
  find_package(Boost COMPONENTS stacktrace REQUIRED)
  target_link_libraries(${PROJECT_NAME}
    PUBLIC
      fmt::fmt
      cctz::cctz
      Boost::stacktrace
    PRIVATE
      yaml-cpp
      cryptopp::cryptopp
      rapidjson
  )
else()
  add_subdirectory("${USERVER_THIRD_PARTY_DIRS}/boost_stacktrace" boost_stacktrace_build)
  target_link_libraries(${PROJECT_NAME}
    PUBLIC
      fmt
      cctz
      userver-stacktrace
    PRIVATE
      libyamlcpp
      CryptoPP
  )
endif()

target_include_directories(${PROJECT_NAME}
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${USERVER_THIRD_PARTY_DIRS}/date/include
    ${USERVER_THIRD_PARTY_DIRS}/function_backports/include
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE
  ${USERVER_THIRD_PARTY_DIRS}/rapidjson/include
)


if (USERVER_IS_THE_ROOT_PROJECT OR USERVER_FEATURE_UTEST)
  add_library(${PROJECT_NAME}-internal-utest INTERFACE)
  add_library(${PROJECT_NAME}-internal-ubench INTERFACE)
  if (USERVER_CONAN)
    find_package(GTest REQUIRED)
    find_package(benchmark REQUIRED)
    target_link_libraries(${PROJECT_NAME}-internal-utest
      INTERFACE
        GTest::gtest
        GTest::gmock
        GTest::gtest_main
    )
    target_link_libraries(${PROJECT_NAME}-internal-ubench
      INTERFACE
        benchmark::benchmark
    )
  else()
    include(SetupGTest)
    include(SetupGBench)
    target_link_libraries(${PROJECT_NAME}-internal-utest
      INTERFACE
        libgtest
        libgmock
    )
    target_link_libraries(${PROJECT_NAME}-internal-ubench
      INTERFACE
        libbenchmark
    )
  endif()
endif()

if (USERVER_IS_THE_ROOT_PROJECT)
    add_library(${PROJECT_NAME}-internal STATIC ${INTERNAL_SOURCES})
    target_compile_definitions(${PROJECT_NAME}-internal PUBLIC $<TARGET_PROPERTY:${PROJECT_NAME},COMPILE_DEFINITIONS>)
    target_include_directories(${PROJECT_NAME}-internal PUBLIC
      $<TARGET_PROPERTY:${PROJECT_NAME},INCLUDE_DIRECTORIES>
      ${CMAKE_CURRENT_SOURCE_DIR}/internal/include
    )
    target_link_libraries(${PROJECT_NAME}-internal
      PUBLIC
        ${PROJECT_NAME}
    )

    add_executable(${PROJECT_NAME}_unittest ${UNIT_TEST_SOURCES})
    target_include_directories (${PROJECT_NAME}_unittest SYSTEM PRIVATE
        $<TARGET_PROPERTY:${PROJECT_NAME},INCLUDE_DIRECTORIES>
    )
    target_link_libraries(${PROJECT_NAME}_unittest
      PUBLIC
        ${PROJECT_NAME}
      PRIVATE
        Boost::program_options
        ${PROJECT_NAME}-internal
        ${PROJECT_NAME}-internal-utest
    )

    # We keep testing deprecated functions, no need to warn about that
    target_compile_options(${PROJECT_NAME}_unittest PRIVATE "-Wno-deprecated-declarations")
    add_google_tests(${PROJECT_NAME}_unittest)

    add_executable(${PROJECT_NAME}_benchmark ${BENCH_SOURCES})
    #    find_package(absl REQUIRED)

    target_include_directories (${PROJECT_NAME}_benchmark SYSTEM PRIVATE
        $<TARGET_PROPERTY:${PROJECT_NAME},INCLUDE_DIRECTORIES>
    )
    target_link_libraries(${PROJECT_NAME}_benchmark
      PUBLIC ${PROJECT_NAME}
        ${PROJECT_NAME}-internal
        ${PROJECT_NAME}-internal-ubench
      )

    option(USERVER_HEADER_MAP_AGAINST_OTHERS_BENCHMARK "build HeaderMap benchmarks against abseil and boost" OFF)
    if (USERVER_HEADER_MAP_AGAINST_OTHERS_BENCHMARK)
      find_package(absl REQUIRED)
      target_link_libraries(${PROJECT_NAME}_benchmark PRIVATE absl::raw_hash_set)
      target_compile_definitions(${PROJECT_NAME}_benchmark PRIVATE HEADER_MAP_AGAINST_OTHERS_BENCHMARK)
    endif()

    add_google_benchmark_tests(${PROJECT_NAME}_benchmark)
endif()
