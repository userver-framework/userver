project(userver-postgresql CXX)

find_package(Boost REQUIRED regex)

find_package(HelperPostgreSQL)

add_subdirectory(pq-extra)

file(GLOB_RECURSE SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)

file(GLOB_RECURSE UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*_test.cpp
)
list(REMOVE_ITEM SOURCES ${UNIT_TEST_SOURCES})

file(GLOB_RECURSE PG_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*_pgtest.cpp
)
list(REMOVE_ITEM SOURCES ${PG_TEST_SOURCES})

file(GLOB_RECURSE BENCH_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*_benchmark.cpp
)
list (REMOVE_ITEM SOURCES ${BENCH_SOURCES})


add_library(${PROJECT_NAME} STATIC ${SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(${PROJECT_NAME} PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_include_directories (${PROJECT_NAME} PRIVATE
    $<TARGET_PROPERTY:userver-core,INCLUDE_DIRECTORIES>
)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    userver-core
  PRIVATE
    userver-pq-extra
    PostgreSQL
    Boost::regex
)



add_executable(${PROJECT_NAME}_unittest ${UNIT_TEST_SOURCES})
target_include_directories (${PROJECT_NAME}_unittest PRIVATE
    $<TARGET_PROPERTY:${PROJECT_NAME},INCLUDE_DIRECTORIES>
)
target_link_libraries(${PROJECT_NAME}_unittest PUBLIC
  userver-utest
  ${PROJECT_NAME}
)
add_google_tests(${PROJECT_NAME}_unittest)

add_executable(${PROJECT_NAME}_benchmark ${BENCH_SOURCES})
target_link_libraries(${PROJECT_NAME}_benchmark PRIVATE
    userver-ubench
    ${PROJECT_NAME}
    userver-pq-extra
    PostgreSQL
)
target_include_directories(${PROJECT_NAME}_benchmark
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# XXX: testsuite doesn't support parallel builds
if(NOT USERVICES)
  add_executable(${PROJECT_NAME}_pgtest ${PG_TEST_SOURCES})
  target_include_directories (${PROJECT_NAME}_pgtest PRIVATE
      $<TARGET_PROPERTY:${PROJECT_NAME},INCLUDE_DIRECTORIES>
  )
  target_link_libraries(${PROJECT_NAME}_pgtest userver-utest ${PROJECT_NAME})
  add_test(${PROJECT_NAME}_pgtest
    env
      POSTGRES_TEST_DSN=postgresql://testsuite@localhost:5433/postgres
      ${CMAKE_BINARY_DIR}/testsuite/taxi-env
      --services=postgresql
      run --
      ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_pgtest
      --gtest_output=xml:${CMAKE_BINARY_DIR}/test-results/${PROJECT_NAME}_pgtest.xml
)
endif(NOT USERVICES)

# Target with no need to use userver namespace, but includes require userver/
add_library(yandex-${PROJECT_NAME} INTERFACE)
target_link_libraries(yandex-${PROJECT_NAME} INTERFACE ${PROJECT_NAME})
