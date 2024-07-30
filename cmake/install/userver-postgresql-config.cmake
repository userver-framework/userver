include_guard(GLOBAL)

if(userver_postgresql_FOUND)
  return()
endif()

find_package(userver REQUIRED COMPONENTS
    core
)

find_package(PostgreSQL REQUIRED)

if(EXISTS "${USERVER_CMAKE_DIR}/SetupPostgresqlDeps.cmake")
  message(STATUS "libpq patch applied")
  include("${USERVER_CMAKE_DIR}/SetupPostgresqlDeps.cmake")
else()
  message(STATUS "libpq patches disabled")
endif()

set(userver_postgresql_FOUND TRUE)
