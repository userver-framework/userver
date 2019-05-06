cmake_minimum_required(VERSION 3.12)

project(yandex-taxi-userver-core)

include(GetUserverVersion)
message(STATUS "Userver version ${USERVER_VERSION}")

include(CheckFunctionExists)
check_function_exists("accept4" HAVE_ACCEPT4)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/build_config.hpp.in
  ${CMAKE_CURRENT_BINARY_DIR}/build_config.hpp
  ESCAPE_QUOTES @ONLY)

file(GLOB_RECURSE SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)

file(GLOB_RECURSE UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/../backend-common-cpp/src/*_test.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/*_test.cpp)

file(GLOB_RECURSE LIBUTEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/testing/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/testing/*.hpp
)

list(REMOVE_ITEM SOURCES ${UNIT_TEST_SOURCES} ${LIBUTEST_SOURCES})

file(GLOB_RECURSE BENCH_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/../backend-common-cpp/*_benchmark.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/*_benchmark.cpp
)
file(GLOB_RECURSE LIBUBENCH_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/benchmark/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/benchmark/*.hpp
)
list (REMOVE_ITEM SOURCES ${BENCH_SOURCES} ${LIBUBENCH_SOURCES})

set(CMAKE_THREAD_PREFER_PTHREAD ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_package(Boost REQUIRED COMPONENTS coroutine program_options filesystem locale thread)
append_debian_depends("libboost-coroutine-dev")
append_debian_depends("libboost-filesystem-dev")
append_debian_depends("libboost-program-options-dev")
append_debian_depends("libboost-locale-dev")
append_debian_depends("libboost-thread-dev")

# backend-common-cpp deps
append_debian_depends("libcctz-dev")

find_package_required(LibEv "libev-dev")
find_package_required(ZLIB "zlib1g-dev")
find_package(ExternalProjectGTest REQUIRED)
find_package(ExternalProjectGBench REQUIRED)
find_package_required(CurlYandex "libyandex-taxi-curl4-openssl-dev libnghttp2-dev libgcrypt11-dev libkrb5-dev")
find_package_required(CryptoPP "libcrypto++-dev")
find_package(Iconv REQUIRED)
find_package_required(YamlCpp "libyaml-cpp-dev")
find_package_required(OpenSSL "libssl-dev")
find_package_required(Fmt "libfmt-dev")

find_package_required_version(Http_Parser
    "libhttp-parser-dev" 2.8)

add_library(${PROJECT_NAME} STATIC ${SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

# https://github.com/jemalloc/jemalloc/issues/820
if(NOT SANITIZE)
  find_package_required(Jemalloc "libyandex-taxi-jemalloc-dev")
  target_link_libraries(${PROJECT_NAME} PUBLIC Jemalloc)
endif(NOT SANITIZE)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    Threads::Threads
    sanitize-target
    yandex-taxi-backend-common-cpp
    libyamlcpp
    Fmt
    CurlYandex
  PRIVATE
    Boost::coroutine
    Boost::filesystem
    Boost::program_options
    CryptoPP
    Http_Parser
    Iconv::Iconv
    LibEv
    OpenSSL::Crypto
    ZLIB::ZLIB
    spdlog_lib
)

set(USERVER_PUBLIC_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(
  USERVER_STANDALONE_ENGINE_INCLUDE_DIR
  ${CMAKE_CURRENT_SOURCE_DIR}/standalone_engine_include
)

target_include_directories(${PROJECT_NAME} PUBLIC
  ${USERVER_PUBLIC_INCLUDE_DIR}
)

target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE
  ${USERVER_THIRD_PARTY_DIRS}/boost-sync-event/include
  ${USERVER_THIRD_PARTY_DIRS}/moodycamel/include
)

target_include_directories(${PROJECT_NAME}
  PRIVATE
    ${USERVER_STANDALONE_ENGINE_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/
    ${CMAKE_CURRENT_BINARY_DIR}
)

set(LIBSTANDALONE_ENGINE yandex-taxi-userver+standalone_engine)
add_library(${LIBSTANDALONE_ENGINE} INTERFACE)
target_link_libraries(${LIBSTANDALONE_ENGINE} INTERFACE ${PROJECT_NAME})
target_include_directories(${LIBSTANDALONE_ENGINE} INTERFACE
  ${USERVER_STANDALONE_ENGINE_INCLUDE_DIR}
)

option(USERVER_PROFILER "Enable task execution profiler" OFF)
message("engine profiler: ${USERVER_PROFILER}")
if (USERVER_PROFILER)
  target_compile_definitions(${PROJECT_NAME} PRIVATE -DUSERVER_PROFILER=1)
  target_compile_definitions(${LIBSTANDALONE_ENGINE} INTERFACE -DUSERVER_PROFILER=1)
endif()
target_compile_definitions(${PROJECT_NAME} PRIVATE -DUSERVER_NO_DEPRECATED_HTTP_ERRORS)

add_library(utest STATIC ${LIBUTEST_SOURCES})
set_target_properties(utest PROPERTIES LINKER_LANGUAGE CXX)
target_compile_definitions(utest PUBLIC $<TARGET_PROPERTY:${PROJECT_NAME},COMPILE_DEFINITIONS>)

target_link_libraries(utest
  PUBLIC
    libgtest
    libgmock
  PRIVATE
    Boost::program_options
)
if (NOT MACOS)
    target_link_libraries(utest
      PUBLIC
        -Wl,--start-group
	    ${LIBSTANDALONE_ENGINE}
            yandex-taxi-backend-common-cpp
        -Wl,--end-group
    )
    target_link_libraries(utest PRIVATE
      atomic
    )
else()
    target_link_libraries(utest
      PUBLIC
	    ${LIBSTANDALONE_ENGINE}
            yandex-taxi-backend-common-cpp
    )
endif()

target_include_directories(utest PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/testing
)

message("gtest: ${GTEST_LIBRARIES}")
add_executable(${PROJECT_NAME}_unittest ${UNIT_TEST_SOURCES})
target_include_directories (${PROJECT_NAME}_unittest SYSTEM PRIVATE
    $<TARGET_PROPERTY:${PROJECT_NAME},INCLUDE_DIRECTORIES>
)
target_link_libraries(${PROJECT_NAME}_unittest
  utest
)
add_google_tests(${PROJECT_NAME}_unittest)


add_library(ubench ${LIBUBENCH_SOURCES})
target_include_directories(ubench PUBLIC ${GBENCH_INCLUDE_DIRS})
target_include_directories(ubench SYSTEM PUBLIC $<TARGET_PROPERTY:${PROJECT_NAME},INCLUDE_DIRECTORIES>)
target_compile_definitions(ubench PUBLIC $<TARGET_PROPERTY:${PROJECT_NAME},COMPILE_DEFINITIONS>)
target_link_libraries(ubench PUBLIC ${GBENCH_LIBRARIES})
if (NOT MACOS)
    target_link_libraries(ubench
        PUBLIC
            atomic
            -Wl,--start-group
	        ${LIBSTANDALONE_ENGINE}
                yandex-taxi-backend-common-cpp
            -Wl,--end-group
    )
else()
    target_link_libraries(ubench
        PUBLIC
	    ${LIBSTANDALONE_ENGINE}
            yandex-taxi-backend-common-cpp
    )
endif()

add_executable(${PROJECT_NAME}_benchmark ${BENCH_SOURCES})
set_target_properties(${PROJECT_NAME}_benchmark PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(${PROJECT_NAME}_benchmark PUBLIC ubench)
