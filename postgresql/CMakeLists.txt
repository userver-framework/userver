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

# Versions of postgresql-server and libpq may diverge "in the wild",
# and we'd like to still be able to build userver postgres in such environments.
# When using APIs of libpq, we sometimes need its version as a macro to know
# which features are available. PG_VERSION_NUM is not suitable, because it is
# provided by postgresql-server, not by libpq. There is PQlibVersion function,
# but no version macro in libpq itself. Workaround: run a simple program
# at build time that calls PQlibVersion.
add_subdirectory(libpq-version-generator)
set(LIBPQ_VERSION_DIR "${CMAKE_CURRENT_BINARY_DIR}/libpq_version")
add_custom_command(
    OUTPUT "${LIBPQ_VERSION_DIR}/userver_libpq_version.hpp"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${LIBPQ_VERSION_DIR}"
    COMMAND $<TARGET_FILE:userver-impl-libpq-version-generator>
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Running gRPC C++ protocol buffer compiler for ${root_path}"
)

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

target_include_directories(${PROJECT_NAME}
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${LIBPQ_VERSION_DIR}"
)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    userver-core
  PRIVATE
    userver-libpq
    Boost::regex
)

target_sources(${PROJECT_NAME} PRIVATE "${LIBPQ_VERSION_DIR}/userver_libpq_version.hpp")

_userver_directory_install(COMPONENT postgres
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/..
)
_userver_install_targets(COMPONENT postgres TARGETS ${PROJECT_NAME} userver-libpq)

_userver_directory_install(COMPONENT postgres FILES
    "${USERVER_ROOT_DIR}/cmake/install/userver-postgresql-config.cmake"
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
      "${CMAKE_BINARY_DIR}/testsuite/env"
      --databases=postgresql
      run --
      $<TARGET_FILE:${PROJECT_NAME}-benchmark>
      --benchmark_min_time=0
      --benchmark_color=no
  )

  add_executable(${PROJECT_NAME}-pgtest ${PG_TEST_SOURCES})
  target_include_directories (${PROJECT_NAME}-pgtest PRIVATE
      $<TARGET_PROPERTY:${PROJECT_NAME},INCLUDE_DIRECTORIES>
      $<TARGET_PROPERTY:userver-core,INCLUDE_DIRECTORIES>
  )
  target_link_libraries(${PROJECT_NAME}-pgtest userver-utest ${PROJECT_NAME})
  add_test(NAME ${PROJECT_NAME}-pgtest COMMAND env
      POSTGRES_TEST_DSN=postgresql://testsuite@localhost:15433/postgres
      "${CMAKE_BINARY_DIR}/testsuite/env"
      --databases=postgresql
      run --
      $<TARGET_FILE:${PROJECT_NAME}-pgtest>
      "--gtest_output=xml:${CMAKE_BINARY_DIR}/test-results/${PROJECT_NAME}-pgtest.xml"
  )

  add_subdirectory(functional_tests)
endif()
