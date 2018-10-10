cmake_minimum_required(VERSION 3.8)

project(yandex-taxi-userver)

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
find_package(ExternalProjectGTest REQUIRED)
find_package(ExternalProjectGBench REQUIRED)
find_package(ExternalProjectOpenTracing REQUIRED)
# TODO: fix in TAXICOMMON-149
# find_package(ExternalProjectJaegerClientCpp REQUIRED)
find_package(CurlYandex REQUIRED)

add_library(${PROJECT_NAME} STATIC ${SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    ${CMAKE_THREAD_LIBS_INIT}
    ${JEMALLOC_LIBRARIES}
    ${LIBBSONCXX_STATIC_LIBRARIES}
    ${LIBMONGOCXX_STATIC_LIBRARIES}
    sanitize-target
    yandex-taxi-backend-common-cpp
    jsoncpp_lib # FIXME move to private in TAXICOMMON-15
    libopentracing
    # libjaegertracing
  PRIVATE
    spdlog_lib
    ${LIBCURL_YANDEX_LIBRARIES}
    ${ZLIB_LIBRARIES}
    Boost::coroutine
    ${HIREDIS_LIBRARIES}
    ${HTTP_PARSER_LIBRARIES}
    ${LIBEV_LIBRARIES}
    ${BSON_STATIC_LIBRARIES}
    ${MONGOC_STATIC_LIBRARIES}
)

set(USERVER_PUBLIC_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../include)
set(
  USERVER_STANDALONE_ENGINE_INCLUDE_DIR
  ${CMAKE_CURRENT_SOURCE_DIR}/../standalone_engine_include
)

target_include_directories(${PROJECT_NAME} PUBLIC
  ${USERVER_PUBLIC_INCLUDE_DIR}
)

# include them as system to suppress errors
target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC
  ${LIBBSONCXX_STATIC_INCLUDE_DIRS}
  ${LIBMONGOCXX_STATIC_INCLUDE_DIRS}
)

target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE
  ${BSON_STATIC_INCLUDE_DIRS}
  ${MONGOC_STATIC_INCLUDE_DIRS}
)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    # Curl include dirs is a public property because curl-ev/easy.hpp
    # uses native curl from the following include:
    ${LIBCURL_YANDEX_INCLUDE_DIR}
  PRIVATE
    ${USERVER_STANDALONE_ENGINE_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${LIBEV_INCLUDE_DIR}
)

add_library(${PROJECT_NAME}+standalone_engine INTERFACE)
target_link_libraries(${PROJECT_NAME}+standalone_engine INTERFACE ${PROJECT_NAME})
target_include_directories(${PROJECT_NAME}+standalone_engine INTERFACE
  ${USERVER_STANDALONE_ENGINE_INCLUDE_DIR}
)

add_library(utest STATIC ${LIBUTEST_SOURCES})
set_target_properties(utest PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(utest
  PUBLIC
    -Wl,--start-group
        ${PROJECT_NAME}+standalone_engine
        yandex-taxi-backend-common-cpp
    -Wl,--end-group
    libgtest
    libgmock
  PRIVATE
    Boost::program_options
)

target_include_directories(utest PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/testing
)

message("gtest: ${GTEST_LIBRARIES}")
add_executable(${PROJECT_NAME}_unittest ${UNIT_TEST_SOURCES})
target_include_directories (${PROJECT_NAME}_unittest SYSTEM PRIVATE
    $<TARGET_PROPERTY:${PROJECT_NAME},INCLUDE_DIRECTORIES>
)
target_link_libraries(${PROJECT_NAME}_unittest
  atomic
  utest
  Boost::program_options
)

add_google_tests(${PROJECT_NAME}_unittest)

add_executable(${PROJECT_NAME}_benchmark ${BENCH_SOURCES})
target_include_directories (${PROJECT_NAME}_benchmark SYSTEM PRIVATE
    $<TARGET_PROPERTY:${PROJECT_NAME},INCLUDE_DIRECTORIES>
)
target_include_directories(${PROJECT_NAME}_benchmark SYSTEM PRIVATE
  ${GBENCH_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}_benchmark
  atomic
  -Wl,--start-group
  ${PROJECT_NAME}
  yandex-taxi-backend-common-cpp
  -Wl,--end-group
  ${GBENCH_LIBRARIES}
)
