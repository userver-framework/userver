project(yandex-taxi-userver-redis)

file(GLOB_RECURSE SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp
)

file(GLOB_RECURSE REDIS_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*_redistest.cpp
)
list(REMOVE_ITEM SOURCES ${REDIS_TEST_SOURCES})

find_package(HelperHiredis)

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

# XXX: testsuite doesn't support parallel builds
if(NOT USERVICES)
  add_executable(${PROJECT_NAME}_redistest ${REDIS_TEST_SOURCES})
  set_target_properties(${PROJECT_NAME}_redistest PROPERTIES LINKER_LANGUAGE CXX)
  target_include_directories (${PROJECT_NAME}_redistest SYSTEM PRIVATE
      $<TARGET_PROPERTY:yandex-taxi-userver-redis,INCLUDE_DIRECTORIES>
  )
  target_link_libraries(${PROJECT_NAME}_redistest utest ${PROJECT_NAME})
  add_test(${PROJECT_NAME}_redistest
    env
      ${CMAKE_BINARY_DIR}/testsuite/taxi-env
      --services=redis
      run --
      ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_redistest
      --gtest_output=xml:${CMAKE_BINARY_DIR}/test-results/${PROJECT_NAME}_redistest.xml
  )
endif(NOT USERVICES)

add_subdirectory(tools/redisclient)
