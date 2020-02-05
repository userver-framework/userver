cmake_minimum_required(VERSION 3.12)

project(yandex-taxi-userver-universal)

if(NOT TARGET yandex-taxi-userver-core)
  # minimal setup
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

  # backend-common-cpp
  # TODO: remove after TAXICOMMON-1949
  set(DISABLE_STACKTRACE ON CACHE BOOL "" FORCE)
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../backend-common-cpp ${CMAKE_CURRENT_BINARY_DIR}/backend-common-cpp)

  target_include_directories(
    yandex-taxi-backend-common-cpp
    PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}/include
  )
  set_property(TARGET yandex-taxi-backend-common-cpp PROPERTY POSITION_INDEPENDENT_CODE ON)
  set_property(TARGET yandex-taxi-jsoncpp PROPERTY POSITION_INDEPENDENT_CODE ON)

  # external-deps codegen
  set(userver_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../)
  include(UserverCodegen)

  # local modules
  include(AddGoogleTests)
  include(FindPackageRequired)
  include(Sanitizers)
endif(NOT TARGET yandex-taxi-userver-core)

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
find_package(Boost REQUIRED COMPONENTS program_options filesystem locale thread)
# backend-common-cpp deps
find_package_required(ZLIB "zlib1g-dev")
find_package(Helpergtest REQUIRED)
find_package(Helpergmock REQUIRED)
find_package(Helpergbench REQUIRED)
find_package_required(CryptoPP "libcrypto++-dev")
find_package(Iconv REQUIRED)
find_package_required(libyamlcpp "libyaml-cpp-dev")
find_package_required(OpenSSL "libssl-dev")
find_package_required(Fmt "libfmt-dev")

find_package_required_version(Http_Parser
    "libhttp-parser-dev" 2.8)

add_library(${PROJECT_NAME} STATIC ${SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES
  LINKER_LANGUAGE CXX
  POSITION_INDEPENDENT_CODE ON
)

# https://github.com/jemalloc/jemalloc/issues/820
if(NOT SANITIZE)
  find_package_required(Jemalloc "libyandex-taxi-jemalloc-dev")
  target_link_libraries(${PROJECT_NAME} PUBLIC Jemalloc)
  # disabled in https://st.yandex-team.ru/TAXICOMMON-1623
  if (NOT MACOS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE JEMALLOC_ENABLED)
  endif(NOT MACOS)
endif(NOT SANITIZE)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    Threads::Threads
    sanitize-target
    libyamlcpp
    Fmt
    yandex-taxi-backend-common-cpp
  PRIVATE
    Boost::filesystem
    Boost::program_options
    CryptoPP
    Iconv::Iconv
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

set(UNIVERSAL_THIRD_PARTY_DIR
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party
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
  PRIVATE
    Boost::program_options
)
if (NOT MACOS)
  target_link_libraries(${PROJECT_NAME}_unittest
    PUBLIC
      -Wl,--start-group
        ${PROJECT_NAME}
      -Wl,--end-group
    PRIVATE
      atomic
  )
else()
  target_link_libraries(${PROJECT_NAME}_unittest
    PUBLIC
      ${PROJECT_NAME}
  )
endif()
add_google_tests(${PROJECT_NAME}_unittest)


add_executable(${PROJECT_NAME}_benchmark ${BENCH_SOURCES})
target_include_directories (${PROJECT_NAME}_benchmark SYSTEM PRIVATE
    $<TARGET_PROPERTY:${PROJECT_NAME},INCLUDE_DIRECTORIES>
)
target_link_libraries(${PROJECT_NAME}_benchmark PUBLIC libbenchmark)
if (NOT MACOS)
  target_link_libraries(${PROJECT_NAME}_benchmark
    PUBLIC
      -Wl,--start-group
        ${PROJECT_NAME}
      -Wl,--end-group
    PRIVATE
      atomic
  )
else()
  target_link_libraries(${PROJECT_NAME}_benchmark
    PUBLIC
      ${PROJECT_NAME}
  )
endif()
