project(userver-mysql CXX)

file(GLOB_RECURSE SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)

file(GLOB_RECURSE MYSQL_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*_mysqltest.cpp
)
list(REMOVE_ITEM SOURCES ${MYSQL_TEST_SOURCES})

add_library(${PROJECT_NAME} STATIC ${SOURCES})

find_package(libmariadb)

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
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE
  ${USERVER_THIRD_PARTY_DIRS}/date/include
)

if (USERVER_IS_THE_ROOT_PROJECT)
  add_executable(${PROJECT_NAME}_mysqltest ${MYSQL_TEST_SOURCES})
  target_include_directories(${PROJECT_NAME}_mysqltest PRIVATE
    $<TARGET_PROPERTY:${PROJECT_NAME},INCLUDE_DIRECTORIES>
  )
  target_link_libraries(${PROJECT_NAME}_mysqltest
    userver-utest
    ${PROJECT_NAME}
    mariadb)
  add_test(${PROJECT_NAME}_mysqltest
    env
      ${CMAKE_BINARY_DIR}/testsuite/env
      --databases=mysql
      run --
      ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_mysqltest
      --gtest_output=xml:${CMAKE_BINARY_DIR}/test-results/${PROJECT_NAME}_mysqltest.xml
  )
endif()
