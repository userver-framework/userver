project(userver-mongo CXX)

if (USERVER_OPEN_SOURCE_BUILD)
  find_package(bson REQUIRED)
  find_package(mongoc REQUIRED)
else()
  find_package(Helperbson-1.0)
  find_package(Helpermongoc-1.0)

  # Unify link names
  set_target_properties(mongo::bson_static PROPERTIES IMPORTED_GLOBAL TRUE)
  set_target_properties(mongo::mongoc_static PROPERTIES IMPORTED_GLOBAL TRUE)
  add_library(bson ALIAS mongo::bson_static)      
  add_library(mongoc ALIAS mongo::mongoc_static)
endif()

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

userver_add_library(${PROJECT_NAME} SOURCES ${SOURCES})

userver_target_link_libraries(${PROJECT_NAME}
  PUBLIC
    userver-core
    bson
  PRIVATE
    mongoc
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

if (USERVER_IS_THE_ROOT_PROJECT)
  add_executable(${PROJECT_NAME}_unittest ${UNIT_TEST_SOURCES})
  target_include_directories (${PROJECT_NAME}_unittest PRIVATE
    $<TARGET_PROPERTY:userver-core,INCLUDE_DIRECTORIES>
  )
  target_link_libraries(${PROJECT_NAME}_unittest userver-utest ${PROJECT_NAME})

  if (USERVER_OPEN_SOURCE_BUILD)
    target_compile_definitions(${PROJECT_NAME}_unittest
      PUBLIC
        -DUSERVER_FEATURE_NO_PATCHED_BSON=1
    )
  endif()

  add_google_tests(${PROJECT_NAME}_unittest)

  add_executable(${PROJECT_NAME}_benchmark ${BENCH_SOURCES})
  target_link_libraries(${PROJECT_NAME}_benchmark PUBLIC userver-ubench ${PROJECT_NAME})
  add_google_benchmark_tests(${PROJECT_NAME}_benchmark)

  add_executable(${PROJECT_NAME}_mongotest ${MONGO_TEST_SOURCES})
  target_include_directories(${PROJECT_NAME}_mongotest PRIVATE
    $<TARGET_PROPERTY:${PROJECT_NAME},INCLUDE_DIRECTORIES>
  )
  target_link_libraries(${PROJECT_NAME}_mongotest userver-utest ${PROJECT_NAME})
  add_test(NAME ${PROJECT_NAME}_mongotest COMMAND env
    ${CMAKE_BINARY_DIR}/testsuite/env
    --databases=mongo
    run --
    $<TARGET_FILE:${PROJECT_NAME}_mongotest>
    --gtest_output=xml:${CMAKE_BINARY_DIR}/test-results/${PROJECT_NAME}_mongotest.xml
  )
endif()

# Target with no need to use userver namespace, but includes require userver/
add_library(yandex-${PROJECT_NAME} INTERFACE)
target_link_libraries(yandex-${PROJECT_NAME} INTERFACE ${PROJECT_NAME})

userver_install(TARGETS ${PROJECT_NAME})
