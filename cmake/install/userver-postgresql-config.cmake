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
    # Without the libpq patches we link against the shared libpq, whose internal
    # symbols (pgcommon/pgport/ldap) are resolved inside the .so. A bare
    # find_package(PostgreSQL) may pick up the static libpq.a instead, which would
    # leave those internal symbols undefined at link time. Force the shared library,
    # mirroring postgresql/CMakeLists.txt.
    if(NOT USERVER_CONAN)
        find_library(PostgreSQL_LIBRARY NAMES libpq.so libpq.dylib REQUIRED)
    endif()
    find_package(PostgreSQL REQUIRED)
endif()

include("${USERVER_CMAKE_DIR}/UserverSql.cmake")

set(userver_postgresql_FOUND TRUE)
