project(userver-mysql CXX)

file(GLOB_RECURSE SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)

file(GLOB_RECURSE MYSQL_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*_mysqltest.cpp
)
list(REMOVE_ITEM SOURCES ${MYSQL_TEST_SOURCES})

add_library(${PROJECT_NAME} STATIC ${SOURCES})

find_package(Mariadbclient)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    userver-core
)
target_include_directories(
  ${PROJECT_NAME}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_include_directories (${PROJECT_NAME} PRIVATE
  $<TARGET_PROPERTY:userver-core,INCLUDE_DIRECTORIES>
)

if (USERVER_IS_THE_ROOT_PROJECT)
  add_executable(${PROJECT_NAME}_mysqltest ${MYSQL_TEST_SOURCES})
  target_include_directories(${PROJECT_NAME}_mysqltest PRIVATE
    $<TARGET_PROPERTY:${PROJECT_NAME},INCLUDE_DIRECTORIES>
  )
  target_link_libraries(${PROJECT_NAME}_mysqltest
    userver-utest
    ${PROJECT_NAME}
    mariadbclient)
#  add_test(${PROJECT_NAME}_chtest
#        env
#        ${CMAKE_BINARY_DIR}/testsuite/env
#        --databases=clickhouse
#        run --
#        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_chtest
#        --gtest_output=xml:${CMAKE_BINARY_DIR}/test-results/${PROJECT_NAME}_chtest.xml
#        )
#set_tests_properties(${PROJECT_NAME}_chtest PROPERTIES ENVIRONMENT
#        "TESTSUITE_CLICKHOUSE_SERVER_START_TIMEOUT=10.0")
endif()
