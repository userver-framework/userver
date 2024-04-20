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
    ${CMAKE_CURRENT_SOURCE_DIR}/src/storages/postgres/portal.cpp
    PROPERTIES COMPILE_FLAGS -DUSERVER_NO_LIBPQ_PATCHES=1
  )

  find_package(PostgreSQL REQUIRED)
  target_link_libraries(userver-libpq INTERFACE PostgreSQL::PostgreSQL)
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
add_library(userver::postgresql ALIAS ${PROJECT_NAME})

set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(${PROJECT_NAME} PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
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

_userver_directory_install(COMPONENT postgres
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/..
)
_userver_install_targets(COMPONENT postgres TARGETS ${PROJECT_NAME} userver-libpq)

_userver_directory_install(COMPONENT postgres FILES
    "${USERVER_ROOT_DIR}/cmake/install/userver-postgresql-config.cmake"
	"${USERVER_ROOT_DIR}/cmake/SetupPostgresqlDeps.cmake"
    "${CMAKE_BINARY_DIR}/cmake_generated/FindPostgreSQLInternal.cmake"
    "${CMAKE_BINARY_DIR}/cmake_generated/FindGssApi.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/userver
)


if (USERVER_IS_THE_ROOT_PROJECT)
  add_executable(${PROJECT_NAME}-unittest ${UNIT_TEST_SOURCES})
  target_include_directories (${PROJECT_NAME}-unittest PRIVATE
      $<TARGET_PROPERTY:${PROJECT_NAME},INCLUDE_DIRECTORIES>
  )
  target_link_libraries(${PROJECT_NAME}-unittest PUBLIC
    userver-utest
    ${PROJECT_NAME}
  )
  add_google_tests(${PROJECT_NAME}-unittest)

  add_executable(${PROJECT_NAME}-benchmark ${BENCH_SOURCES})
  target_link_libraries(${PROJECT_NAME}-benchmark PRIVATE
      userver-ubench
      ${PROJECT_NAME}
      userver-libpq
  )
  target_include_directories(${PROJECT_NAME}-benchmark
      PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/src
  )
  add_test(NAME ${PROJECT_NAME}-benchmark COMMAND env
      POSTGRES_DSN_BENCH=postgresql://testsuite@localhost:15433/postgres
      ${CMAKE_BINARY_DIR}/testsuite/env
      --databases=postgresql
      run --
      $<TARGET_FILE:${PROJECT_NAME}-benchmark>
      --benchmark_min_time=0
      --benchmark_color=no
  )

  add_executable(${PROJECT_NAME}-pgtest ${PG_TEST_SOURCES})
  target_include_directories (${PROJECT_NAME}-pgtest PRIVATE
      $<TARGET_PROPERTY:${PROJECT_NAME},INCLUDE_DIRECTORIES>
  )
  target_link_libraries(${PROJECT_NAME}-pgtest userver-utest ${PROJECT_NAME})
  add_test(NAME ${PROJECT_NAME}_pgtest COMMAND env
      POSTGRES_TEST_DSN=postgresql://testsuite@localhost:15433/postgres
      ${CMAKE_BINARY_DIR}/testsuite/env
      --databases=postgresql
      run --
      $<TARGET_FILE:${PROJECT_NAME}-pgtest>
      --gtest_output=xml:${CMAKE_BINARY_DIR}/test-results/${PROJECT_NAME}_pgtest.xml
  )

  add_subdirectory(functional_tests)
endif()


