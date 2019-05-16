project(yandex-taxi-userver-redis)

file(GLOB_RECURSE SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp
)

find_package(HelperHiredis)
append_debian_depends("libhiredis-dev")

add_library(${PROJECT_NAME} STATIC ${SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

set(REDIS_COMMON yandex-taxi-backend-common-cpp-redis)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    yandex-taxi-userver-core
    ${REDIS_COMMON}
    Hiredis
  PRIVATE
    yandex-taxi-userver-uboost-coro # uses Future
)
target_include_directories(
  ${PROJECT_NAME}
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_include_directories (${PROJECT_NAME} SYSTEM PRIVATE
    $<TARGET_PROPERTY:yandex-taxi-userver-core,INCLUDE_DIRECTORIES>
)

add_subdirectory(../backend-common-cpp/redis ${CMAKE_CURRENT_BINARY_DIR}/backend-common-cpp-redis/)
target_include_directories(
  ${REDIS_COMMON}
  PUBLIC
    ../core/include/
  PRIVATE
    ../core/src/
    ../uboost_coro/include/
    ${SPDLOG_INCLUDE_DIRS}
)

file(GLOB_RECURSE REDIS_COMMON_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/../backend-common-cpp/redis/src/*_test.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../backend-common-cpp/src/testing/*_test.cpp
)

add_executable(${REDIS_COMMON}_unittest ${REDIS_COMMON_TEST_SOURCES})
set_target_properties(${REDIS_COMMON}_unittest PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(${REDIS_COMMON}_unittest
  utest
  yandex-taxi-userver-core
  ${REDIS_COMMON}
  Hiredis
)
target_include_directories(
  ${REDIS_COMMON}_unittest
  PUBLIC
    ../core/include/
  PRIVATE
    ../core/src/
    ${SPDLOG_INCLUDE_DIRS}
)
add_google_tests(${REDIS_COMMON}_unittest)

add_subdirectory(tools/redisclient)
