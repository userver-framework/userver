project(userver-postgresql CXX)

find_package(Boost REQUIRED regex)


option(USERVER_FEATURE_PATCH_LIBPQ "Apply patches to the libpq (add portals support)" ON)
add_library(userver-libpq INTERFACE)
if (USERVER_FEATURE_PATCH_LIBPQ)
  add_subdirectory(pq-extra)
  target_link_libraries(userver-libpq INTERFACE userver-pq-extra)
else()
  set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/storages/postgres/detail/pg_connection_wrapper.cpp
    PROPERTIES COMPILE_FLAGS -DUSERVER_NO_LIBPQ_PATCHES=1
  )

  find_library(LIBPQ_LIBRARIES NAMES libpq.a libpq.so)
  if (NOT LIBPQ_LIBRARIES)
    message(FATAL_ERROR "Failed to find libpq")
  endif()
  target_link_libraries(userver-libpq INTERFACE ${LIBPQ_LIBRARIES})

  find_path(LIBPQ_INCLUDE_DIR
  NAMES libpq-fe.h
  PATH_SUFFIXES
    pgsql
    postgresql
    include
    )
  if (NOT LIBPQ_INCLUDE_DIR)
    message(FATAL_ERROR "Failed to find libpq (headers)")
  endif()
endif()

file(GLOB _PG_CONFIG_HEADERS "${LIBPQ_INCLUDE_DIR}/pg_config*.h")
foreach(_PG_CONFIG_HEADER ${_PG_CONFIG_HEADERS})
  if(EXISTS "${_PG_CONFIG_HEADER}")
    file(STRINGS "${_PG_CONFIG_HEADER}" pgsql_version_str
         REGEX "^#define[\t ]+PG_VERSION_NUM[\t ]+.*")
    if(pgsql_version_str)
      string(REGEX REPLACE "^#define[\t ]+PG_VERSION_NUM[\t ]+([0-9]*).*"
             "\\1" LIBPQ_VERSION_NUM "${pgsql_version_str}")
      break()
    endif()
  endif()
endforeach()
message("Found PostgreSQL version (PG_VERSION_NUM): ${LIBPQ_VERSION_NUM}")

if(PG_VERSION_NUM GREATER_EQUAL 140005)
  find_library(LIBPQ_LIBPGCOMMON NAMES libpgcommon.a)
  find_library(LIBPQ_LIBPGPORT NAMES libpgport.a)
  if(NOT LIBPQ_LIBPGCOMMON OR NOT LIBPQ_LIBPGPORT)
    message(FATAL_ERROR "Not found libpgcommon.a/libpgport.a")
  endif()
  list(APPEND LIBPQ_LIBRARIES ${LIBPQ_LIBPGCOMMON})
  list(APPEND LIBPQ_LIBRARIES ${LIBPQ_LIBPGPORT})
endif()


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

file(GLOB_RECURSE PG_FUNCTIONAL_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/functional_tests/*
)
list(REMOVE_ITEM SOURCES ${PG_FUNCTIONAL_TEST_SOURCES})

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
    userver-libpq
    Boost::regex
)

if (USERVER_IS_THE_ROOT_PROJECT)
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
      userver-libpq
  )
  target_include_directories(${PROJECT_NAME}_benchmark
      PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/src
  )
  add_test(NAME ${PROJECT_NAME}_benchmark COMMAND env
      POSTGRES_DSN_BENCH=postgresql://testsuite@localhost:15433/postgres
      ${CMAKE_BINARY_DIR}/testsuite/env
      --databases=postgresql
      run --
      $<TARGET_FILE:${PROJECT_NAME}_benchmark>
      --benchmark_min_time=0
      --benchmark_color=no
  )

  add_executable(${PROJECT_NAME}_pgtest ${PG_TEST_SOURCES})
  target_include_directories (${PROJECT_NAME}_pgtest PRIVATE
      $<TARGET_PROPERTY:${PROJECT_NAME},INCLUDE_DIRECTORIES>
  )
  target_link_libraries(${PROJECT_NAME}_pgtest userver-utest ${PROJECT_NAME})
  add_test(NAME ${PROJECT_NAME}_pgtest COMMAND env
      POSTGRES_TEST_DSN=postgresql://testsuite@localhost:15433/postgres
      ${CMAKE_BINARY_DIR}/testsuite/env
      --databases=postgresql
      run --
      $<TARGET_FILE:${PROJECT_NAME}_pgtest>
      --gtest_output=xml:${CMAKE_BINARY_DIR}/test-results/${PROJECT_NAME}_pgtest.xml
  )

  add_subdirectory(functional_tests)
endif()

# Target with no need to use userver namespace, but includes require userver/
add_library(yandex-${PROJECT_NAME} INTERFACE)
target_link_libraries(yandex-${PROJECT_NAME} INTERFACE ${PROJECT_NAME})
