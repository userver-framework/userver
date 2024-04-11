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
add_library(userver::mysql ALIAS ${PROJECT_NAME})

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
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

_userver_directory_install(COMPONENT mysql
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/..
)
_userver_install_targets(COMPONENT mysql TARGETS ${PROJECT_NAME})

_userver_directory_install(COMPONENT mysql FILES
    "${USERVER_ROOT_DIR}/cmake/install/userver-mysql-config.cmake"
    "${CMAKE_BINARY_DIR}/cmake_generated/Findlibmariadb.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/userver
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

  add_executable(${PROJECT_NAME}-benchmark ${MYSQL_BENCH_SOURCES})
  target_include_directories(${PROJECT_NAME}-benchmark PRIVATE
    $<TARGET_PROPERTY:${PROJECT_NAME},INCLUDE_DIRECTORIES>
  )
  target_link_libraries(${PROJECT_NAME}-benchmark PRIVATE userver-ubench ${PROJECT_NAME}
          mariadb
    )

  add_subdirectory(functional_tests)
endif()
