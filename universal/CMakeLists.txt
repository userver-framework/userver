project(userver-universal CXX)

if (NOT USERVER_ROOT_DIR)
  message(FATAL_ERROR "Include userver project rather than the userver/universal")
endif()

if (TARGET userver-core)
  message(FATAL_ERROR "userver-core should be included after userver-universal")
endif()

include(Sanitizers)
_userver_make_sanitize_target()

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
find_package(Boost REQUIRED
    COMPONENTS
    program_options
    filesystem
    regex
    stacktrace_basic
    OPTIONAL_COMPONENTS
    stacktrace_backtrace
)
include(UserverRequireDWCAS)  # Should be called after `find_package(Boost REQUIRED)`

message(STATUS "boost: ${Boost_VERSION_STRING}")
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  if (CMAKE_SYSTEM_NAME MATCHES "Darwin" AND Boost_FOUND)
    # requires Boost_FOUND to make a valid expression
    if (${Boost_VERSION_STRING} VERSION_LESS "1.68.0")
      message(FATAL_ERROR "Boost Locale version less that 1.68 uses features deleted from standard. Please update your boost distribution.")
    endif()
  endif()
endif()

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

# Compiler flags for userver code. Flags for all the code
# including third_party are in cmake/UserverSetupEnvironment.cmake
add_library(userver-internal-compile-options INTERFACE)
userver_target_require_dwcas(userver-internal-compile-options INTERFACE)
target_compile_features(userver-internal-compile-options INTERFACE cxx_std_17)
target_compile_options(userver-internal-compile-options INTERFACE
  "-Wall" "-Wextra" "-Wpedantic"
)

include(UserverCxxCompileOptionsIfSupported)
userver_target_cxx_compile_options_if_supported(userver-internal-compile-options INTERFACE
  "-ftemplate-backtrace-limit=0"

  # -Wall and -Wextra do not enable these
  "-Wdisabled-optimization" "-Winvalid-pch" "-Wimplicit-fallthrough"
  "-Wlogical-op" "-Wformat=2" "-Wno-error=deprecated-declarations"  

  # This warning is unavoidable in generic code with templates
  "-Wno-useless-cast"

  # Starting from C++20, passing zero arguments to a variadic macro parameter is allowed
  "-Wno-gnu-zero-variadic-macro-arguments"
)

# Gives false positives
if (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang"
    AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 12
    AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 13)
  target_compile_options(userver-internal-compile-options INTERFACE "-Wno-range-loop-analysis")
endif()

# Trimming filename prefixes 
get_filename_component(BASE_PREFIX "${USERVER_ROOT_DIR}/../" ABSOLUTE)
file(TO_NATIVE_PATH "${CMAKE_SOURCE_DIR}/" SRC_LOG_PATH_BASE)
file(TO_NATIVE_PATH "${CMAKE_BINARY_DIR}/" BIN_LOG_PATH_BASE)

# Depending on OS and CMake version the path may or may not have a trailing '/'
if (NOT SRC_LOG_PATH_BASE MATCHES ".*/$")
  set(SRC_LOG_PATH_BASE "${SRC_LOG_PATH_BASE}/")
endif()
if (NOT BIN_LOG_PATH_BASE MATCHES ".*/$")
  set(BIN_LOG_PATH_BASE "${BIN_LOG_PATH_BASE}/")
endif()
if (NOT BASE_PREFIX STREQUAL "/" AND NOT BASE_PREFIX MATCHES ".*/$")
  set(BASE_PREFIX "${BASE_PREFIX}/")
endif()

userver_is_cxx_compile_option_supported(USERVER_COMPILER_HAS_MACRO_PREFIX_MAP "-fmacro-prefix-map=a=b")
if (USERVER_COMPILER_HAS_MACRO_PREFIX_MAP)
  target_compile_options(userver-internal-compile-options INTERFACE
    -fmacro-prefix-map=${BIN_LOG_PATH_BASE}=
    -fmacro-prefix-map=${BASE_PREFIX}=
  )
  if (NOT SRC_LOG_PATH_BASE MATCHES "${BASE_PREFIX}.*")
    target_compile_options(userver-internal-compile-options INTERFACE
      -fmacro-prefix-map=${SRC_LOG_PATH_BASE}=
    )
  endif()
else()
  target_compile_definitions(userver-internal-compile-options INTERFACE
    USERVER_LOG_SOURCE_PATH_BASE=${SRC_LOG_PATH_BASE}
    USERVER_LOG_BUILD_PATH_BASE=${BIN_LOG_PATH_BASE}
    USERVER_LOG_PREFIX_PATH_BASE=${BASE_PREFIX}
  )
endif()

# Check stdlib is recent enough
include(CheckIncludeFileCXX)
check_include_file_cxx(variant HAS_CXX17_VARIANT)
if(NOT HAS_CXX17_VARIANT)
  message(FATAL_ERROR "You have an outdated standard C++ library")
endif()

option(USERVER_NO_WERROR "Do not treat warnings as errors" ON)
if (NOT USERVER_NO_WERROR)
  message(STATUS "Forcing warnings as errors!")
  target_compile_options(userver-internal-compile-options INTERFACE "-Werror")
endif()

_userver_install_targets(COMPONENT universal 
  TARGETS userver-internal-sanitize-options userver-internal-compile-options)

# The library itself
add_library(${PROJECT_NAME} STATIC ${SOURCES})

include(GetUserverVersion)
set(USERVER_NAMESPACE "userver" CACHE STRING "C++ namespace to use")
if(NOT "${USERVER_NAMESPACE}" STREQUAL "")
  set(USERVER_NAMESPACE_BEGIN "namespace ${USERVER_NAMESPACE} { inline namespace ${USERVER_VERSION_STR} { " CACHE STRING "Open C++ namespace to use")
  set(USERVER_NAMESPACE_END "} }" CACHE STRING "Close C++ namespace to use")
endif()
message(STATUS "Putting userver into namespace '${USERVER_NAMESPACE}': ${USERVER_NAMESPACE_BEGIN} ${USERVER_NAMESPACE_END}")
target_compile_definitions(${PROJECT_NAME} PUBLIC
  "USERVER_NAMESPACE=${USERVER_NAMESPACE}"
  "USERVER_NAMESPACE_BEGIN=${USERVER_NAMESPACE_BEGIN}"
  "USERVER_NAMESPACE_END=${USERVER_NAMESPACE_END}"
  "USERVER=1"
)

# https://github.com/jemalloc/jemalloc/issues/820
if (USERVER_FEATURE_JEMALLOC AND NOT USERVER_SANITIZE AND NOT CMAKE_SYSTEM_NAME MATCHES "Darwin")
  if (USERVER_CONAN)
    find_package(jemalloc REQUIRED)
    target_link_libraries(${PROJECT_NAME} PUBLIC jemalloc::jemalloc)
  else()
    find_package_required(Jemalloc "libjemalloc-dev")
    target_link_libraries(${PROJECT_NAME} PUBLIC Jemalloc)
  endif()
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE
  CRYPTOPP_ENABLE_NAMESPACE_WEAK=1
)

set(USERVER_LOG_LEVEL_ENUM "trace, info, debug, warning, error")
set(USERVER_FEATURE_ERASE_LOG_WITH_LEVEL_DEFAULT "")

set(USERVER_FEATURE_ERASE_LOG_WITH_LEVEL ${USERVER_FEATURE_ERASE_LOG_WITH_LEVEL_DEFAULT} CACHE STRING "Logs of this and below levels are removed from binary, possible values: ${USERVER_LOG_LEVEL_ENUM}")
set(USERVER_ERASE_LOG_WITH_LEVEL_PENDING ${USERVER_FEATURE_ERASE_LOG_WITH_LEVEL})
separate_arguments(USERVER_ERASE_LOG_WITH_LEVEL_PENDING)
list(REMOVE_DUPLICATES USERVER_ERASE_LOG_WITH_LEVEL_PENDING)

if("error" IN_LIST USERVER_ERASE_LOG_WITH_LEVEL_PENDING)
  target_compile_definitions(${PROJECT_NAME} PUBLIC
      "USERVER_FEATURE_ERASE_LOG_WITH_LEVEL=4"
  )
elseif("warning" IN_LIST USERVER_ERASE_LOG_WITH_LEVEL_PENDING)
  target_compile_definitions(${PROJECT_NAME} PUBLIC
      "USERVER_FEATURE_ERASE_LOG_WITH_LEVEL=3"
  )
elseif("info" IN_LIST USERVER_ERASE_LOG_WITH_LEVEL_PENDING)
  target_compile_definitions(${PROJECT_NAME} PUBLIC
      "USERVER_FEATURE_ERASE_LOG_WITH_LEVEL=2"
  )
elseif("debug" IN_LIST USERVER_ERASE_LOG_WITH_LEVEL_PENDING)
  target_compile_definitions(${PROJECT_NAME} PUBLIC
      "USERVER_FEATURE_ERASE_LOG_WITH_LEVEL=1"
  )
elseif("trace" IN_LIST USERVER_ERASE_LOG_WITH_LEVEL_PENDING)
  target_compile_definitions(${PROJECT_NAME} PUBLIC
      "USERVER_FEATURE_ERASE_LOG_WITH_LEVEL=0"
  )
endif()

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
    userver-internal-sanitize-options
    userver-internal-compile-options
  PRIVATE
    Boost::filesystem
    Boost::program_options
    Boost::regex
    OpenSSL::Crypto
    OpenSSL::SSL
)

if (USERVER_CONAN)
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
  include(Stacktrace)
  _make_stacktrace_target(userver-stacktrace "${Boost_VERSION_STRING}")
  _userver_install_targets(COMPONENT universal TARGETS userver-stacktrace)

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
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${USERVER_THIRD_PARTY_DIRS}/date/include>
    $<BUILD_INTERFACE:${USERVER_THIRD_PARTY_DIRS}/function_backports/include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/
    ${CMAKE_CURRENT_BINARY_DIR}
)
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE
  ${USERVER_THIRD_PARTY_DIRS}/rapidjson/include
)

add_library(userver::universal ALIAS ${PROJECT_NAME})
_userver_directory_install(COMPONENT universal
  DIRECTORY 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${USERVER_THIRD_PARTY_DIRS}/date/include
    ${USERVER_THIRD_PARTY_DIRS}/function_backports/include
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/..
)
_userver_install_targets(COMPONENT universal TARGETS ${PROJECT_NAME})

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
        ${PROJECT_NAME}
    )
    target_link_libraries(${PROJECT_NAME}-internal-ubench
      INTERFACE
        benchmark::benchmark
        ${PROJECT_NAME}
    )
  else()
    include(SetupGTest)
    include(SetupGBench)
    target_link_libraries(${PROJECT_NAME}-internal-utest
      INTERFACE
        libgtest
        libgmock
        ${PROJECT_NAME}
    )
    target_link_libraries(${PROJECT_NAME}-internal-ubench
      INTERFACE
        libbenchmark
        ${PROJECT_NAME}
    )
  endif()
endif()

if (USERVER_IS_THE_ROOT_PROJECT)
    add_library(${PROJECT_NAME}-internal OBJECT ${INTERNAL_SOURCES})
    target_compile_definitions(${PROJECT_NAME}-internal PUBLIC $<TARGET_PROPERTY:${PROJECT_NAME},COMPILE_DEFINITIONS>)
    target_include_directories(${PROJECT_NAME}-internal PUBLIC
      $<TARGET_PROPERTY:${PROJECT_NAME},INCLUDE_DIRECTORIES>
      ${CMAKE_CURRENT_SOURCE_DIR}/internal/include
    )
    target_link_libraries(${PROJECT_NAME}-internal
      PUBLIC
        ${PROJECT_NAME}
    )

    add_executable(${PROJECT_NAME}-unittest ${UNIT_TEST_SOURCES})
    target_include_directories (${PROJECT_NAME}-unittest SYSTEM PRIVATE
        $<TARGET_PROPERTY:${PROJECT_NAME},INCLUDE_DIRECTORIES>
    )
    target_link_libraries(${PROJECT_NAME}-unittest
      PUBLIC
        ${PROJECT_NAME}
      PRIVATE
        Boost::program_options
        ${PROJECT_NAME}-internal
        ${PROJECT_NAME}-internal-utest
    )

    add_google_tests(${PROJECT_NAME}-unittest)

    add_executable(${PROJECT_NAME}-benchmark ${BENCH_SOURCES})

    target_include_directories (${PROJECT_NAME}-benchmark SYSTEM PRIVATE
        $<TARGET_PROPERTY:${PROJECT_NAME},INCLUDE_DIRECTORIES>
    )
    target_link_libraries(${PROJECT_NAME}-benchmark
      PUBLIC ${PROJECT_NAME}
        ${PROJECT_NAME}-internal
        ${PROJECT_NAME}-internal-ubench
      )

    option(USERVER_HEADER_MAP_AGAINST_OTHERS_BENCHMARK "build HeaderMap benchmarks against abseil and boost" OFF)
    if (USERVER_HEADER_MAP_AGAINST_OTHERS_BENCHMARK)
      find_package(absl REQUIRED)
      target_link_libraries(${PROJECT_NAME}-benchmark PRIVATE absl::raw_hash_set)
      target_compile_definitions(${PROJECT_NAME}-benchmark PRIVATE HEADER_MAP_AGAINST_OTHERS_BENCHMARK)
    endif()

    add_google_benchmark_tests(${PROJECT_NAME}-benchmark)
endif()

if (USERVER_INSTALL)
  configure_package_config_file("${USERVER_ROOT_DIR}/cmake/install/Config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/userverConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/userver
  )

  write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/userverConfigVersion.cmake
    VERSION ${USERVER_VERSION} COMPATIBILITY SameMajorVersion
  )

  _userver_directory_install(COMPONENT universal FILES
      "${CMAKE_CURRENT_BINARY_DIR}/userverConfig.cmake"
      "${CMAKE_CURRENT_BINARY_DIR}/userverConfigVersion.cmake"
      "${CMAKE_BINARY_DIR}/cmake_generated/FindCryptoPP.cmake"
      "${CMAKE_BINARY_DIR}/cmake_generated/Findlibyamlcpp.cmake"
      "${CMAKE_BINARY_DIR}/cmake_generated/Findcctz.cmake"
      "${CMAKE_BINARY_DIR}/cmake_generated/FindJemalloc.cmake"
      "${USERVER_ROOT_DIR}/cmake/sanitize.blacklist.txt"
      "${USERVER_ROOT_DIR}/cmake/sanitize-macos.blacklist.txt"
      "${USERVER_ROOT_DIR}/cmake/RequireLTO.cmake"
      "${USERVER_ROOT_DIR}/cmake/SetupLTO.cmake"
      "${USERVER_ROOT_DIR}/cmake/SetupLinker.cmake"
      "${USERVER_ROOT_DIR}/cmake/Sanitizers.cmake"
      "${USERVER_ROOT_DIR}/cmake/AddGoogleTests.cmake"
      "${USERVER_ROOT_DIR}/cmake/DetectVersion.cmake"
      "${USERVER_ROOT_DIR}/cmake/UserverSetupEnvironment.cmake"
      "${USERVER_ROOT_DIR}/cmake/install/userver-universal-config.cmake"
      DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/userver
  )
endif()
