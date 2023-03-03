project(userver-mysql CXX)

file(GLOB_RECURSE SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)

file(GLOB_RECURSE MYSQL_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*_mysqltest.cpp
)
list(REMOVE_ITEM SOURCES ${MYSQL_TEST_SOURCES})

file(GLOB_RECURSE MYSQL_BENCH_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*_mysqlbench.cpp
)
list(REMOVE_ITEM SOURCES ${MYSQL_BENCH_SOURCES})
list(APPEND MYSQL_BENCH_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/storages/tests/utils_mysqltest.cpp
)

add_library(${PROJECT_NAME} STATIC ${SOURCES})

find_package(libmariadb REQUIRED)
#add_subdirectory(${USERVER_ROOT_DIR}/third_party/mariadb-connector-c "${CMAKE_BINARY_DIR}/third_party/mariadb-connector-c")
#target_include_directories(${PROJECT_NAME} PRIVATE
#  ${USERVER_THIRD_PARTY_DIRS}/mariadb-connector-c/include
#)
#target_include_directories(${PROJECT_NAME} PRIVATE
#  ${CMAKE_BINARY_DIR}/third_party/mariadb-connector-c/include
#)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    userver-core
)
target_include_directories(
  ${PROJECT_NAME}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(${PROJECT_NAME} PUBLIC -DUSERVER_MYSQL_ASSERT_WHEN_IMPLEMENTED)

if (USERVER_IS_THE_ROOT_PROJECT)
  add_executable(${PROJECT_NAME}_mysqltest ${MYSQL_TEST_SOURCES})
  target_include_directories(${PROJECT_NAME}_mysqltest PRIVATE
    $<TARGET_PROPERTY:${PROJECT_NAME},INCLUDE_DIRECTORIES>
  )
  target_link_libraries(${PROJECT_NAME}_mysqltest userver-utest ${PROJECT_NAME}
          #mariadbclient
          mariadb
    )
  set(TESTS_TEMPLATE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src/storages/tests/tests_runner.sh)
  set(UNIT_TESTS_PATH ${CMAKE_CURRENT_BINARY_DIR}/unit_tests.sh)
  set(TESTS_BINARY_PATH ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_mysqltest)
  set(TESTS_ARGS --gtest_output=xml:${CMAKE_BINARY_DIR}/test-results/${PROJECT_NAME}_mysqltest.xml)
  execute_process(COMMAND cp ${TESTS_TEMPLATE_PATH} ${UNIT_TESTS_PATH})
  execute_process(COMMAND sed -i "s%__BINARY__%${TESTS_BINARY_PATH}%" ${UNIT_TESTS_PATH})
  execute_process(COMMAND sed -i "s%__ARGS__%${TESTS_ARGS}%" ${UNIT_TESTS_PATH})
  add_test(${PROJECT_NAME}_mysqltest
    env
      ${CMAKE_BINARY_DIR}/testsuite/env
      --databases=mysql
      run --
      ${UNIT_TESTS_PATH}
  )

  add_executable(${PROJECT_NAME}_benchmark ${MYSQL_BENCH_SOURCES})
  target_include_directories(${PROJECT_NAME}_benchmark PRIVATE
    $<TARGET_PROPERTY:${PROJECT_NAME},INCLUDE_DIRECTORIES>
  )
  target_link_libraries(${PROJECT_NAME}_benchmark PRIVATE userver-ubench ${PROJECT_NAME}
          #mariadbclient
          mariadb
    )
#  add_test(${PROJECT_NAME}_mysqltest
#    env
#      ${CMAKE_BINARY_DIR}/testsuite/env
#      --databases=mysql
#      run --
#      ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_benchmark
#      --benchmark_min_time=0
#      --benchmark_color=no
#  )
endif()
