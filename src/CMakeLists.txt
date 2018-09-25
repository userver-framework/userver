cmake_minimum_required(VERSION 3.5)

set(PROJECT yandex-taxi-userver)
project(${PROJECT} VERSION ${PROJECT_VERSION_STRING})

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/build_config.hpp.in
  ${CMAKE_CURRENT_BINARY_DIR}/build_config.hpp
  ESCAPE_QUOTES @ONLY)

file(GLOB_RECURSE SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)

file(GLOB_RECURSE UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/../backend-common-cpp/*_test.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/*_test.cpp)

file(GLOB_RECURSE LIBUTEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/testing/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/testing/*.hpp)

list (REMOVE_ITEM SOURCES ${UNIT_TEST_SOURCES} ${LIBUTEST_SOURCES})

file(GLOB_RECURSE BENCH_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/../backend-common-cpp/*_benchmark.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/*_benchmark.cpp)
list (REMOVE_ITEM SOURCES ${BENCH_SOURCES})


set(CMAKE_THREAD_PREFER_PTHREAD ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_package(Boost REQUIRED COMPONENTS coroutine program_options)
find_package(Jemalloc REQUIRED)
find_package(Hiredis 0.13.3 REQUIRED)
find_package(HttpParser REQUIRED)
find_package(jsoncpp REQUIRED)
find_package(LibEv REQUIRED)
find_package(libbson-static-1.0 1.12.0 REQUIRED)
find_package(libmongoc-static-1.0 1.12.0 REQUIRED)
find_package(libbsoncxx-static 3.3.1 REQUIRED)
find_package(libmongocxx-static 3.3.1 REQUIRED)
find_package(ZLIB REQUIRED)
find_package(ExternalProjectGTest)
find_package(ExternalProjectGBench)
find_package(ExternalProjectOpenTracing)
find_package(CurlYandex REQUIRED)

add_library(${PROJECT} STATIC ${SOURCES})
set_target_properties(${PROJECT} PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(${PROJECT}
  PUBLIC
    ${CMAKE_THREAD_LIBS_INIT}
    ${JEMALLOC_LIBRARIES}
    ${LIBBSONCXX_STATIC_LIBRARIES}
    ${LIBMONGOCXX_STATIC_LIBRARIES}
    yandex-taxi-backend-common-cpp
    jsoncpp_lib # FIXME move to private in TAXICOMMON-15
    libopentracing
  PRIVATE
    spdlog_lib
    ${LIBCURL_YANDEX_LIBRARIES}
    ${ZLIB_LIBRARIES}
    Boost::coroutine
    ${HIREDIS_LIBRARIES}
    ${HTTP_PARSER_LIBRARIES}
    ${LIBEV_LIBRARIES}
    ${CLANG_LIBRARIES}
    ${BSON_STATIC_LIBRARIES}
    ${MONGOC_STATIC_LIBRARIES}
)

set(USERVER_PUBLIC_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../include)

target_include_directories(${PROJECT} PUBLIC
  ${USERVER_PUBLIC_INCLUDE_DIR}
)

# include them as system to suppress errors
target_include_directories(${PROJECT} SYSTEM PUBLIC
  ${LIBBSONCXX_STATIC_INCLUDE_DIRS}
  ${LIBMONGOCXX_STATIC_INCLUDE_DIRS}
)

target_include_directories(${PROJECT} SYSTEM PRIVATE
  ${BSON_STATIC_INCLUDE_DIRS}
  ${MONGOC_STATIC_INCLUDE_DIRS}
)

# FIXME: change to PRIVATE once we have interface
target_include_directories(${PROJECT} PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${LIBCURL_YANDEX_INCLUDE_DIR}
  ${LIBEV_INCLUDE_DIR}
)

add_library(utest STATIC ${LIBUTEST_SOURCES})
set_target_properties(utest PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(utest
  PUBLIC
    -Wl,--start-group
        ${PROJECT}
        yandex-taxi-backend-common-cpp
    -Wl,--end-group
    libgtest
    libgmock
  PRIVATE
    Boost::program_options
    spdlog_lib
)

target_include_directories(utest PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/testing
)



message("gtest: ${GTEST_LIBRARIES}")
add_executable(${PROJECT}_unittest ${UNIT_TEST_SOURCES})
target_include_directories (${PROJECT}_unittest SYSTEM PRIVATE
    $<TARGET_PROPERTY:${PROJECT},INCLUDE_DIRECTORIES>
)
target_link_libraries(${PROJECT}_unittest
  atomic
  utest
  Boost::program_options
)

add_google_tests(${PROJECT}_unittest)

add_executable(${PROJECT}_benchmark ${BENCH_SOURCES})
target_include_directories (${PROJECT}_benchmark SYSTEM PRIVATE
    $<TARGET_PROPERTY:${PROJECT},INCLUDE_DIRECTORIES>
)
target_include_directories(${PROJECT}_benchmark SYSTEM PRIVATE
  ${GBENCH_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT}_benchmark
  atomic
  -Wl,--start-group
  ${PROJECT}
  yandex-taxi-backend-common-cpp
  -Wl,--end-group
  ${GBENCH_LIBRARIES}
)
