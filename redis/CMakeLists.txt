project(userver-redis CXX)

file(GLOB_RECURSE SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp
)

file(GLOB_RECURSE UTESTLIB_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/testing/src/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/testing/src/*.hpp
)
list(REMOVE_ITEM SOURCES ${UTESTLIB_SOURCES})

file(GLOB_RECURSE UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*_test.cpp
)
list(REMOVE_ITEM SOURCES ${UNIT_TEST_SOURCES})

file(GLOB_RECURSE REDIS_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*_redistest.cpp
)
list(REMOVE_ITEM SOURCES ${REDIS_TEST_SOURCES})

find_package(Hiredis)

userver_add_library(${PROJECT_NAME} SOURCES ${SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

# for libev
set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/storages/redis/impl/redis.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/storages/redis/impl/sentinel_impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/storages/redis/impl/subscription_rebalance_scheduler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/storages/redis/impl/ev_thread/thread.cpp
    PROPERTIES COMPILE_FLAGS -fno-strict-aliasing
)
target_compile_options (${PROJECT_NAME} PRIVATE -Wno-old-style-cast)

option(USERVER_FEATURE_REDIS_HI_MALLOC "Provide 'hi_malloc(unsigned long)' function to workaround https://bugs.launchpad.net/ubuntu/+source/hiredis/+bug/1888025" OFF)
if (USERVER_FEATURE_REDIS_HI_MALLOC)
  set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/storages/redis/impl/hi_malloc.cpp
    PROPERTIES COMPILE_FLAGS -DUSERVER_FEATURE_REDIS_HI_MALLOC=1
  )
endif()

option(USERVER_FEATURE_REDIS_TLS "SSL/TLS support for Redis driver" OFF)
if(USERVER_FEATURE_REDIS_TLS)
  add_definitions(-DUSERVER_FEATURE_REDIS_TLS=1)
endif()

userver_target_link_libraries(${PROJECT_NAME}
  PUBLIC
    userver-core
    Hiredis
  PRIVATE
    userver-uboost-coro # uses BlockingFuture
)

if (USERVER_FEATURE_REDIS_TLS)
  target_link_libraries(${PROJECT_NAME} PUBLIC hiredis_ssl)
endif()
target_include_directories(
  ${PROJECT_NAME}
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_include_directories (${PROJECT_NAME} PRIVATE
    $<TARGET_PROPERTY:userver-core,INCLUDE_DIRECTORIES>
)

# Helper library for testing user code with redis. Provides mock_client_base.hpp
# and other helper headers
if (USERVER_FEATURE_UTEST)
    userver_add_library(${PROJECT_NAME}_utest SOURCES ${UTESTLIB_SOURCES})
    target_include_directories(
      ${PROJECT_NAME}_utest
      PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/testing/include
    )
    target_include_directories (${PROJECT_NAME}_utest PRIVATE
      $<TARGET_PROPERTY:userver-core,INCLUDE_DIRECTORIES>
    )
    target_link_libraries(${PROJECT_NAME}_utest
      userver-utest
      ${PROJECT_NAME}
    )
endif()

if (USERVER_IS_THE_ROOT_PROJECT)
  add_executable(${PROJECT_NAME}_unittest ${UNIT_TEST_SOURCES})
  target_link_libraries(${PROJECT_NAME}_unittest
    userver-utest
    ${PROJECT_NAME}_utest
    ${PROJECT_NAME}
  )
  target_include_directories(
    ${PROJECT_NAME}_unittest
    PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/src
  )
  target_include_directories (${PROJECT_NAME}_unittest PRIVATE
    $<TARGET_PROPERTY:userver-core,INCLUDE_DIRECTORIES>
  )
  add_google_tests(${PROJECT_NAME}_unittest)

  add_executable(${PROJECT_NAME}_redistest ${REDIS_TEST_SOURCES})
  target_include_directories (${PROJECT_NAME}_redistest PRIVATE
      $<TARGET_PROPERTY:userver-redis,INCLUDE_DIRECTORIES>
  )
  target_link_libraries(${PROJECT_NAME}_redistest userver-utest ${PROJECT_NAME})
  add_test(${PROJECT_NAME}_redistest
    env
      ${CMAKE_BINARY_DIR}/testsuite/env
      --databases=redis
      run --
      ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_redistest
      --gtest_output=xml:${CMAKE_BINARY_DIR}/test-results/${PROJECT_NAME}_redistest.xml
  )

  add_subdirectory(tools/redisclient)
endif()

if (USERVER_FEATURE_UTEST)
    # Target with no need to use userver namespace, but includes require userver/
    add_library(yandex-${PROJECT_NAME}_utest INTERFACE)
    target_link_libraries(yandex-${PROJECT_NAME}_utest INTERFACE ${PROJECT_NAME}_utest)
endif()

# Target with no need to use userver namespace, but includes require userver/
add_library(yandex-${PROJECT_NAME} INTERFACE)
target_link_libraries(yandex-${PROJECT_NAME} INTERFACE ${PROJECT_NAME})

userver_install(TARGETS ${PROJECT_NAME})
