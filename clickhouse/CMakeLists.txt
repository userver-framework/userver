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
  PRIVATE
    userver-compiler-rt-parts
)
target_include_directories(
  ${PROJECT_NAME}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_include_directories(${PROJECT_NAME} PRIVATE
  ${USERVER_THIRD_PARTY_DIRS}/clickhouse-cpp
  ${USERVER_THIRD_PARTY_DIRS}/clickhouse-cpp/contrib
)

if (USERVER_IS_THE_ROOT_PROJECT)
  add_executable(${PROJECT_NAME}_unittest ${UNIT_TEST_SOURCES})
  target_include_directories (${PROJECT_NAME}_unittest PRIVATE
    $<TARGET_PROPERTY:${PROJECT_NAME},INCLUDE_DIRECTORIES>
  )
  target_link_libraries(${PROJECT_NAME}_unittest userver-utest ${PROJECT_NAME})
  add_google_tests(${PROJECT_NAME}_unittest)

  target_compile_definitions(${PROJECT_NAME}_unittest PRIVATE
      DEFAULT_DYNAMIC_CONFIG_FILENAME="${CMAKE_SOURCE_DIR}/clickhouse/tests/dynamic_config_fallback.json"
  )
  
  add_executable(${PROJECT_NAME}_chtest ${CLICKHOUSE_TEST_SOURCES})
  target_include_directories(${PROJECT_NAME}_chtest PRIVATE
    $<TARGET_PROPERTY:${PROJECT_NAME},INCLUDE_DIRECTORIES>
  )
  target_link_libraries(${PROJECT_NAME}_chtest userver-utest ${PROJECT_NAME})
  add_test(${PROJECT_NAME}_chtest
    env
      ${CMAKE_BINARY_DIR}/testsuite/env
      --databases=clickhouse
      run --
      ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_chtest
      --gtest_output=xml:${CMAKE_BINARY_DIR}/test-results/${PROJECT_NAME}_chtest.xml
  )
  set_tests_properties(${PROJECT_NAME}_chtest PROPERTIES ENVIRONMENT
          "TESTSUITE_CLICKHOUSE_SERVER_START_TIMEOUT=30.0")

  add_subdirectory(functional_tests)
endif()
