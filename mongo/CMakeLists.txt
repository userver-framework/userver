project(userver-mongo)

find_package(Helperbson-1.0)
find_package(Helpermongoc-1.0)

file(GLOB_RECURSE SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)

file(GLOB_RECURSE UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*_test.cpp
)
list(REMOVE_ITEM SOURCES ${UNIT_TEST_SOURCES})

file(GLOB_RECURSE MONGO_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*_mongotest.cpp
)
list(REMOVE_ITEM SOURCES ${MONGO_TEST_SOURCES})

file(GLOB_RECURSE BENCH_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*_benchmark.cpp
)
list (REMOVE_ITEM SOURCES ${BENCH_SOURCES})

add_library(${PROJECT_NAME} STATIC ${SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    userver-core
    mongo::bson_static
  PRIVATE
    mongo::mongoc_static
)
target_include_directories(
  ${PROJECT_NAME}
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_include_directories (${PROJECT_NAME} SYSTEM PRIVATE
  $<TARGET_PROPERTY:userver-core,INCLUDE_DIRECTORIES>
)

add_executable(${PROJECT_NAME}_unittest ${UNIT_TEST_SOURCES})
set_target_properties(${PROJECT_NAME}_unittest PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories (${PROJECT_NAME}_unittest SYSTEM PRIVATE
  $<TARGET_PROPERTY:userver-core,INCLUDE_DIRECTORIES>
)
target_link_libraries(${PROJECT_NAME}_unittest utest ${PROJECT_NAME})
add_google_tests(${PROJECT_NAME}_unittest)

add_executable(${PROJECT_NAME}_benchmark ${BENCH_SOURCES})
set_target_properties(${PROJECT_NAME}_benchmark PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(${PROJECT_NAME}_benchmark PUBLIC ubench ${PROJECT_NAME})

# XXX: testsuite doesn't support parallel builds
if(NOT USERVICES)
  add_executable(${PROJECT_NAME}_mongotest ${MONGO_TEST_SOURCES})
  set_target_properties(${PROJECT_NAME}_mongotest PROPERTIES LINKER_LANGUAGE CXX)
  target_link_libraries(${PROJECT_NAME}_mongotest utest ${PROJECT_NAME})
  add_test(${PROJECT_NAME}_mongotest
    env
      ${CMAKE_BINARY_DIR}/testsuite/taxi-env
      --services=mongo
      run --
      ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_mongotest
      --gtest_output=xml:${CMAKE_BINARY_DIR}/test-results/${PROJECT_NAME}_mongotest.xml
)
endif(NOT USERVICES)

# Target with no need to use userver namespace, but includes require userver/
add_library(yandex-${PROJECT_NAME} INTERFACE)
target_link_libraries(yandex-${PROJECT_NAME} INTERFACE ${PROJECT_NAME})

# Target with no need to use userver namespace and includes work even without userver/
add_library(yandex-taxi-${PROJECT_NAME} INTERFACE)
target_link_libraries(yandex-taxi-${PROJECT_NAME} INTERFACE ${PROJECT_NAME})
target_include_directories(yandex-taxi-${PROJECT_NAME} INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/include/userver
)
