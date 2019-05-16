cmake_minimum_required(VERSION 3.8)

project(yandex-taxi-userver-postgresql)

find_package(Boost REQUIRED regex)
append_debian_depends("libboost-regex-dev")

# For time zones in pg driver
find_package_components_required(ICU "libicu-dev" uc i18n)
    
find_package(HelperPostgreSQL)

file(GLOB_RECURSE SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)

file(GLOB_RECURSE UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*_test.cpp
)
list(REMOVE_ITEM SOURCES ${UNIT_TEST_SOURCES})

file(GLOB_RECURSE BENCH_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*_benchmark.cpp
)
list (REMOVE_ITEM SOURCES ${BENCH_SOURCES})


add_library(${PROJECT_NAME} STATIC ${SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${USERVER_THIRD_PARTY_DIRS}/magic_get/include
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_include_directories (${PROJECT_NAME} SYSTEM PRIVATE
    $<TARGET_PROPERTY:yandex-taxi-userver-core,INCLUDE_DIRECTORIES>
)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    yandex-taxi-userver-core
  PRIVATE
    yandex-taxi-userver-uboost-coro # uses TaskProcessor
    PostgreSQL
    ICU::uc
    ICU::i18n
    Boost::regex
)



add_executable(${PROJECT_NAME}_unittest ${UNIT_TEST_SOURCES})
target_include_directories (${PROJECT_NAME}_unittest SYSTEM PRIVATE
    $<TARGET_PROPERTY:${PROJECT_NAME},INCLUDE_DIRECTORIES>
)
target_link_libraries(${PROJECT_NAME}_unittest PUBLIC
  utest
  ${PROJECT_NAME}
)
add_google_tests(${PROJECT_NAME}_unittest)

add_executable(${PROJECT_NAME}_benchmark ${BENCH_SOURCES})
set_target_properties(${PROJECT_NAME}_benchmark PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(${PROJECT_NAME}_benchmark PRIVATE
    ubench
    ${PROJECT_NAME}
)
target_include_directories(${PROJECT_NAME}_benchmark
    SYSTEM PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
