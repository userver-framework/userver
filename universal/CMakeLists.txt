project(userver-universal)

option(USE_JEMALLOC "Enable linkage with jemalloc memory allocator" ON)

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
    ${CMAKE_BINARY_DIR}/cmake/generated
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

  # external-deps codegen
  set(userver_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../)
  include(UserverCodegen)

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
find_package(Boost REQUIRED COMPONENTS program_options filesystem thread regex)
find_package_required(ZLIB "zlib1g-dev")
if (OPEN_SOURCE_BUILD)
  include(SetupGTest)
else()
  find_package(Helpergtest REQUIRED)
endif()

if (OPEN_SOURCE_BUILD)
  include(SetupGBench)
else()
  find_package(Helpergbench REQUIRED)
endif()
find_package_required(CryptoPP "libcrypto++-dev")
find_package(Iconv REQUIRED)
find_package_required(libyamlcpp "libyaml-cpp-dev")
find_package_required(OpenSSL "libssl-dev")
include(SetupFmt)
include(SetupCCTZ)

find_package_required(Http_Parser "libhttp-parser-dev")

add_library(${PROJECT_NAME} STATIC ${SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES
  LINKER_LANGUAGE CXX
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
  POSITION_INDEPENDENT_CODE ON
)

# https://github.com/jemalloc/jemalloc/issues/820
if(USE_JEMALLOC AND NOT SANITIZE)
  find_package_required(Jemalloc "libyandex-taxi-jemalloc-dev")
  target_link_libraries(${PROJECT_NAME} PUBLIC Jemalloc)
  # disabled in https://st.yandex-team.ru/TAXICOMMON-1623
  if (NOT MACOS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE JEMALLOC_ENABLED)
  endif(NOT MACOS)
endif(USE_JEMALLOC AND NOT SANITIZE)

message(STATUS "Putting userver into namespace '${USERVER_NAMESPACE}': ${USERVER_NAMESPACE_BEGIN} ${USERVER_NAMESPACE_END}")
target_compile_definitions(${PROJECT_NAME} PUBLIC
  "USERVER_NAMESPACE=${USERVER_NAMESPACE}"
  "USERVER_NAMESPACE_BEGIN=${USERVER_NAMESPACE_BEGIN}"
  "USERVER_NAMESPACE_END=${USERVER_NAMESPACE_END}"
)

# https://bugs.llvm.org/show_bug.cgi?id=16404
if(SANITIZE AND NOT CMAKE_BUILD_TYPE MATCHES "^Rel")
  target_link_libraries(${PROJECT_NAME} PUBLIC userver-compiler-rt-parts)
endif(SANITIZE AND NOT CMAKE_BUILD_TYPE MATCHES "^Rel")

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    Threads::Threads
    sanitize-target
    libyamlcpp
    fmt
    cctz
    userver-stacktrace
  PRIVATE
    Boost::filesystem
    Boost::program_options
    Boost::regex
    CryptoPP
    OpenSSL::Crypto
    OpenSSL::SSL
    ZLIB::ZLIB
)

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

target_include_directories(${PROJECT_NAME}
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../shared/src/
    ${CMAKE_CURRENT_SOURCE_DIR}/src/
    ${CMAKE_CURRENT_BINARY_DIR}
)


add_executable(${PROJECT_NAME}_unittest ${UNIT_TEST_SOURCES})
target_include_directories (${PROJECT_NAME}_unittest SYSTEM PRIVATE
    $<TARGET_PROPERTY:${PROJECT_NAME},INCLUDE_DIRECTORIES>
)
target_link_libraries(${PROJECT_NAME}_unittest
  PUBLIC
    libgtest
    libgmock
    ${PROJECT_NAME}
  PRIVATE
    Boost::program_options
)

# We keep testing deprecated functions, no need to warn about that
target_compile_options(${PROJECT_NAME}_unittest PRIVATE "-Wno-deprecated-declarations")

add_google_tests(${PROJECT_NAME}_unittest)


add_executable(${PROJECT_NAME}_benchmark ${BENCH_SOURCES})
target_include_directories (${PROJECT_NAME}_benchmark SYSTEM PRIVATE
    $<TARGET_PROPERTY:${PROJECT_NAME},INCLUDE_DIRECTORIES>
)
target_link_libraries(${PROJECT_NAME}_benchmark
  PUBLIC
    libbenchmark
    ${PROJECT_NAME}
)

# Target with no need to use userver namespace, but includes require userver/
add_library(yandex-${PROJECT_NAME} INTERFACE)
target_link_libraries(yandex-${PROJECT_NAME} INTERFACE ${PROJECT_NAME})
