project(userver-universal)

set(USERVER_NAMESPACE "userver" CACHE STRING "C++ namespace to use")
if (NOT "${USERVER_NAMESPACE}" STREQUAL "")
    set(USERVER_NAMESPACE_BEGIN "namespace ${USERVER_NAMESPACE} {" CACHE STRING "Open C++ namespace to use")
    set(USERVER_NAMESPACE_END "}" CACHE STRING "Close C++ namespace to use")
endif()

set(UNIVERSAL_THIRD_PARTY_DIR
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party
)

if(NOT TARGET userver-core)
  # minimal setup
  if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(MACOS found)
  endif()

  cmake_policy(SET CMP0025 NEW)
  if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
    set(CLANG found)
  endif()

  set(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    ${CMAKE_CURRENT_SOURCE_DIR}/../cmake
    ${CMAKE_BINARY_DIR}/cmake_generated
  )

  add_definitions(-DUSERVER_UNIVERSAL)

  # bug on xenial https://bugs.launchpad.net/ubuntu/+source/llvm-toolchain-3.8/+bug/1664321
  add_definitions (-DBOOST_REGEX_NO_EXTERNAL_TEMPLATES=1)

  # boost.stacktrace
  add_subdirectory(${UNIVERSAL_THIRD_PARTY_DIR}/boost_stacktrace ${CMAKE_CURRENT_BINARY_DIR}/boost_stacktrace)
  set_target_properties(userver-stacktrace PROPERTIES
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
  )

  # sanitizers fixups
  add_subdirectory(${UNIVERSAL_THIRD_PARTY_DIR}/compiler-rt ${CMAKE_CURRENT_BINARY_DIR}/compiler-rt)
  set_target_properties(userver-compiler-rt-parts PROPERTIES
    POSITION_INDEPENDENT_CODE ON
  )

  # local modules
  include(AddGoogleTests)
  include(FindPackageRequired)
  include(Sanitizers)
endif(NOT TARGET userver-core)

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
file(GLOB_RECURSE BENCH_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/../shared/*_benchmark.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/*_benchmark.cpp
)
list(REMOVE_ITEM SOURCES ${UNIT_TEST_SOURCES} ${BENCH_SOURCES})

set(CMAKE_THREAD_PREFER_PTHREAD ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_package(Boost REQUIRED COMPONENTS program_options filesystem regex)
find_package_required(ZLIB "zlib1g-dev")

find_package(Iconv REQUIRED)
find_package_required(OpenSSL "libssl-dev")

if (USERVER_CONAN)
  find_package(GTest REQUIRED)
  find_package(benchmark REQUIRED)
  find_package(cryptopp REQUIRED)
  find_package(yaml-cpp REQUIRED)
  find_package(fmt REQUIRED)
  find_package(cctz REQUIRED)

  find_package(RapidJSON REQUIRED)
  target_compile_definitions(rapidjson INTERFACE RAPIDJSON_HAS_STDSTRING)
else()
  include(SetupGTest)
  include(SetupGBench)
  include(SetupCryptoPP)
  find_package_required(libyamlcpp "libyaml-cpp-dev")
  include(SetupFmt)
  include(SetupCCTZ)
endif()

add_library(${PROJECT_NAME} STATIC ${SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES
  LINKER_LANGUAGE CXX
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
  POSITION_INDEPENDENT_CODE ON
)

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

message(STATUS "Putting userver into namespace '${USERVER_NAMESPACE}': ${USERVER_NAMESPACE_BEGIN} ${USERVER_NAMESPACE_END}")
target_compile_definitions(${PROJECT_NAME} PUBLIC
  "USERVER_NAMESPACE=${USERVER_NAMESPACE}"
  "USERVER_NAMESPACE_BEGIN=${USERVER_NAMESPACE_BEGIN}"
  "USERVER_NAMESPACE_END=${USERVER_NAMESPACE_END}"
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
  CRYPTOPP_ENABLE_NAMESPACE_WEAK=1
)

# https://bugs.llvm.org/show_bug.cgi?id=16404
if (USERVER_SANITIZE AND NOT CMAKE_BUILD_TYPE MATCHES "^Rel")
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
    ZLIB::ZLIB
)
if (USERVER_CONAN)
  target_link_libraries(${PROJECT_NAME}
    PUBLIC
      yaml-cpp
      fmt::fmt
      cctz::cctz
      Boost::stacktrace
    PRIVATE
      cryptopp::cryptopp
      rapidjson
  )
else()
  target_link_libraries(${PROJECT_NAME}
    PUBLIC
      libyamlcpp
      fmt
      cctz
      userver-stacktrace
    PRIVATE
      CryptoPP
  )
endif()

set(UNIVERSAL_PUBLIC_INCLUDE_DIRS
  ${CMAKE_CURRENT_SOURCE_DIR}/../shared/include
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_include_directories(${PROJECT_NAME} PUBLIC
  ${UNIVERSAL_PUBLIC_INCLUDE_DIRS}
)

target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE
  ${UNIVERSAL_THIRD_PARTY_DIR}/rapidjson/include
)

target_include_directories(${PROJECT_NAME} PUBLIC
  ${UNIVERSAL_THIRD_PARTY_DIR}/date/include
)

target_include_directories(${PROJECT_NAME}
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../shared/src/
    ${CMAKE_CURRENT_SOURCE_DIR}/src/
    ${CMAKE_CURRENT_BINARY_DIR}
)


if (USERVER_IS_THE_ROOT_PROJECT)
    add_executable(${PROJECT_NAME}_unittest ${UNIT_TEST_SOURCES})
    target_include_directories (${PROJECT_NAME}_unittest SYSTEM PRIVATE
        $<TARGET_PROPERTY:${PROJECT_NAME},INCLUDE_DIRECTORIES>
    )
    target_link_libraries(${PROJECT_NAME}_unittest
      PUBLIC
        ${PROJECT_NAME}
      PRIVATE
        Boost::program_options
    )
    if (USERVER_CONAN)
      target_link_libraries(${PROJECT_NAME}_unittest
        PUBLIC
          GTest::gtest
          GTest::gmock
          GTest::gtest_main
      )
    else()
      target_link_libraries(${PROJECT_NAME}_unittest
        PUBLIC
          libgtest
          libgmock
      )
    endif()

    # We keep testing deprecated functions, no need to warn about that
    target_compile_options(${PROJECT_NAME}_unittest PRIVATE "-Wno-deprecated-declarations")
    add_google_tests(${PROJECT_NAME}_unittest)

    add_executable(${PROJECT_NAME}_benchmark ${BENCH_SOURCES})
    target_include_directories (${PROJECT_NAME}_benchmark SYSTEM PRIVATE
        $<TARGET_PROPERTY:${PROJECT_NAME},INCLUDE_DIRECTORIES>
    )
    target_link_libraries(${PROJECT_NAME}_benchmark PUBLIC ${PROJECT_NAME})
    if (USERVER_CONAN)
      target_link_libraries(${PROJECT_NAME}_benchmark PUBLIC benchmark::benchmark)
    else()
      target_link_libraries(${PROJECT_NAME}_benchmark PUBLIC libbenchmark)
    endif()
    add_google_benchmark_tests(${PROJECT_NAME}_benchmark)
endif()
