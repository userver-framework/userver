include_guard(GLOBAL)

if(userver_postgresql_FOUND)
  return()
endif()

find_package(PostgreSQL REQUIRED)
include("${USERVER_CMAKE_DIR}/SetupPostgresqlDeps.cmake")

add_library(userver::postgresql ALIAS userver::userver-postgresql)

set(userver_postgresql_FOUND TRUE)
