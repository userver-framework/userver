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

if (USERVER_MYSQL_ALLOW_BUGGY_LIBMARIADB)
  target_compile_definitions(${PROJECT_NAME} PRIVATE -DUSERVER_MYSQL_ALLOW_BUGGY_LIBMARIADB)
endif()

find_package(libmariadb REQUIRED)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    userver-core
)
target_include_directories(
  ${PROJECT_NAME}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if (USERVER_IS_THE_ROOT_PROJECT)
  add_executable(${PROJECT_NAME}_mysqltest ${MYSQL_TEST_SOURCES})
  target_include_directories(${PROJECT_NAME}_mysqltest PRIVATE
    $<TARGET_PROPERTY:${PROJECT_NAME},INCLUDE_DIRECTORIES>
  )
  target_link_libraries(${PROJECT_NAME}_mysqltest userver-utest ${PROJECT_NAME}
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
          mariadb
    )
endif()
