project(userver-clickhouse CXX)

file(GLOB_RECURSE SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)

file(GLOB_RECURSE UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*_test.cpp
)
list(REMOVE_ITEM SOURCES ${UNIT_TEST_SOURCES})

file(GLOB_RECURSE CLICKHOUSE_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*_chtest.cpp
)
list(REMOVE_ITEM SOURCES ${CLICKHOUSE_TEST_SOURCES})
list(APPEND CLICKHOUSE_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/storages/tests/utils_test.cpp
)

file(GLOB_RECURSE CH_FUNCTIONAL_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/functional_tests/*
)
list(REMOVE_ITEM SOURCES ${CH_FUNCTIONAL_TEST_SOURCES})

add_library(${PROJECT_NAME} STATIC ${SOURCES})

if (USERVER_CONAN)
  find_package(clickhouse-cpp REQUIRED)
  target_link_libraries(${PROJECT_NAME} PRIVATE clickhouse-cpp-lib::clickhouse-cpp-lib)
else()
  include(SetupClickhouseCPP)
  target_link_libraries(${PROJECT_NAME} PRIVATE clickhouse-cpp)
endif()

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    userver-core
)
target_include_directories(${PROJECT_NAME}
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if (USERVER_IS_THE_ROOT_PROJECT)
  add_executable(${PROJECT_NAME}-unittest ${UNIT_TEST_SOURCES})
  target_include_directories (${PROJECT_NAME}-unittest PRIVATE
    $<TARGET_PROPERTY:${PROJECT_NAME},INCLUDE_DIRECTORIES>
  )
  target_link_libraries(${PROJECT_NAME}-unittest userver-utest ${PROJECT_NAME})
  add_google_tests(${PROJECT_NAME}-unittest)
  
  add_executable(${PROJECT_NAME}-chtest ${CLICKHOUSE_TEST_SOURCES})
  target_include_directories(${PROJECT_NAME}-chtest PRIVATE
    $<TARGET_PROPERTY:${PROJECT_NAME},INCLUDE_DIRECTORIES>
  )
  target_link_libraries(${PROJECT_NAME}-chtest userver-utest ${PROJECT_NAME})
  add_test(${PROJECT_NAME}-chtest
    env
      ${CMAKE_BINARY_DIR}/testsuite/env
      --databases=clickhouse
      run --
      ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-chtest
      --gtest_output=xml:${CMAKE_BINARY_DIR}/test-results/${PROJECT_NAME}_chtest.xml
  )
  set_tests_properties(${PROJECT_NAME}-chtest PROPERTIES ENVIRONMENT
          "TESTSUITE_CLICKHOUSE_SERVER_START_TIMEOUT=120.0")

  add_subdirectory(functional_tests)
endif()
