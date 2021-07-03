project(userver-redis)

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
find_package(Boost REQUIRED COMPONENTS thread)

add_library(${PROJECT_NAME} STATIC ${SOURCES})
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

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    userver-core
    Hiredis
  PRIVATE
    Boost::thread
    userver-uboost-coro # uses BlockingFuture
)
target_include_directories(
  ${PROJECT_NAME}
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/include/userver  # TODO: remove after fixing the paths
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_include_directories (${PROJECT_NAME} SYSTEM PRIVATE
    $<TARGET_PROPERTY:userver-core,INCLUDE_DIRECTORIES>
)


add_library(${PROJECT_NAME}_utest STATIC ${UTESTLIB_SOURCES})
set_target_properties(${PROJECT_NAME}_utest PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(
  ${PROJECT_NAME}_utest
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/testing/include
)
target_include_directories (${PROJECT_NAME}_utest SYSTEM PRIVATE
  $<TARGET_PROPERTY:userver-core,INCLUDE_DIRECTORIES>
)
target_link_libraries(${PROJECT_NAME}_utest
  utest
  ${PROJECT_NAME}
)

add_executable(${PROJECT_NAME}_unittest ${UNIT_TEST_SOURCES})
set_target_properties(${PROJECT_NAME}_unittest PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(${PROJECT_NAME}_unittest
  utest
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
target_include_directories (${PROJECT_NAME}_unittest SYSTEM PRIVATE
  $<TARGET_PROPERTY:userver-core,INCLUDE_DIRECTORIES>
)
add_google_tests(${PROJECT_NAME}_unittest)

# XXX: testsuite doesn't support parallel builds
if(NOT USERVICES)
  add_executable(${PROJECT_NAME}_redistest ${REDIS_TEST_SOURCES})
  set_target_properties(${PROJECT_NAME}_redistest PROPERTIES LINKER_LANGUAGE CXX)
  target_include_directories (${PROJECT_NAME}_redistest SYSTEM PRIVATE
      $<TARGET_PROPERTY:userver-redis,INCLUDE_DIRECTORIES>
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

# Target with no need to use userver namespace, but includes require userver/
add_library(yandex-${PROJECT_NAME}_utest INTERFACE)
target_link_libraries(yandex-${PROJECT_NAME}_utest INTERFACE ${PROJECT_NAME})

# Target with no need to use userver namespace and includes work even without userver/
add_library(yandex-taxi-${PROJECT_NAME}_utest INTERFACE)
target_link_libraries(yandex-taxi-${PROJECT_NAME}_utest INTERFACE ${PROJECT_NAME}_utest)
target_include_directories(yandex-taxi-${PROJECT_NAME}_utest INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/include/userver
)


# Target with no need to use userver namespace, but includes require userver/
add_library(yandex-${PROJECT_NAME} INTERFACE)
target_link_libraries(yandex-${PROJECT_NAME} INTERFACE ${PROJECT_NAME})

# Target with no need to use userver namespace and includes work even without userver/
add_library(yandex-taxi-${PROJECT_NAME} INTERFACE)
target_link_libraries(yandex-taxi-${PROJECT_NAME} INTERFACE ${PROJECT_NAME})
target_include_directories(yandex-taxi-${PROJECT_NAME} INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/include/userver
)
