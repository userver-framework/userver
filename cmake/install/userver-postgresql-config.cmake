include_guard(GLOBAL)

if(userver_postgresql_FOUND)
    return()
endif()

find_package(userver REQUIRED COMPONENTS core)

if(EXISTS "${USERVER_CMAKE_DIR}/SetupPostgresqlDeps.cmake")
    message(STATUS "libpq patch applied")
    include("${USERVER_CMAKE_DIR}/SetupPostgresqlDeps.cmake")
else()
    message(STATUS "libpq patches disabled")
    find_package(PostgreSQL REQUIRED)
endif()

include("${USERVER_CMAKE_DIR}/UserverSql.cmake")

set(userver_postgresql_FOUND TRUE)
