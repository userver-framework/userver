project(userver-postgresql CXX)

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

  if(USERVER_CONAN)
    find_package(PostgreSQL REQUIRED CONFIG)
  else()
    find_library(PostgreSQL_LIBRARY NAMES libpq.so libpq.dylib REQUIRED)
    find_package(PostgreSQL REQUIRED)
  endif()
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

userver_module(postgresql
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
    LINK_LIBRARIES_PRIVATE userver-libpq
    INCLUDE_DIRS_PRIVATE "${LIBPQ_VERSION_DIR}"
    UTEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*_test.cpp"
    DBTEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*_pgtest.cpp"
    DBTEST_DATABASES postgresql
    DBTEST_ENV POSTGRES_TEST_DSN=postgresql://testsuite@localhost:15433/postgres
    UBENCH_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*_benchmark.cpp"
    UBENCH_LINK_LIBRARIES userver-libpq
    UBENCH_DATABASES postgresql
    UBENCH_ENV POSTGRES_DSN_BENCH=postgresql://testsuite@localhost:15433/postgres
)

target_sources(${PROJECT_NAME} PRIVATE "${LIBPQ_VERSION_DIR}/userver_libpq_version.hpp")

_userver_install_targets(COMPONENT postgres TARGETS userver-libpq)
_userver_directory_install(COMPONENT postgres FILES
    "${USERVER_ROOT_DIR}/cmake/UserverSql.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/userver"
)
_userver_directory_install(COMPONENT postgres
    DIRECTORY "${USERVER_ROOT_DIR}/scripts/sql"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/userver/scripts"
)

if (USERVER_BUILD_TESTS)
  add_subdirectory(functional_tests)
endif()

set(USERVER_DIR "${USERVER_ROOT_DIR}")
include(UserverSql)
