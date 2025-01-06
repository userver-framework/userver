_userver_module_begin(
    NAME PostgreSQLInternal
    DEBIAN_NAMES libpq-dev postgresql-server-dev-16
    FORMULA_NAMES postgresql@16
    RPM_NAMES postgresql-server-devel postgresql-static
    PKG_NAMES postgresql16-server
)

_userver_module_find_include(
    NAMES postgres_fe.h
    PATHS
    ${USERVER_PG_SERVER_INCLUDE_DIR}
    /usr/include/postgresql/12/server
    /usr/include/postgresql/13/server
    /usr/include/postgresql/14/server
    /usr/include/postgresql/15/server
    /usr/include/postgresql/16/server
    /usr/include/postgresql/17/server
    /usr/include/postgresql/18/server
    /usr/include/postgresql/19/server
    /usr/include/postgresql/20/server
    /usr/local/include/postgresql/server  # FreeBSD
    /usr/include/postgresql/server  # Manjaro
    PATH_SUFFIXES
    pgsql/server         # postgresql-server-devel
)

_userver_module_find_include(
    NAMES libpq-int.h
    PATHS
    ${USERVER_PG_INCLUDE_DIR}/internal
    ${USERVER_PG_INCLUDE_DIR}/postgresql/internal
    /usr/local/include/postgresql/internal  # FreeBSD
    PATH_SUFFIXES
    postgresql/internal  # libpq-dev
    pgsql/internal       # postgresql-private-devel
)

_userver_module_find_include(
    NAMES libpq-fe.h
    PATHS
    ${USERVER_PG_INCLUDE_DIR}
    /usr/local/include  # FreeBSD
    PATH_SUFFIXES
    postgresql
    pgsql
)

_userver_module_find_library(
    NAMES libpq.a  # must be a static library as we use internal symbols
    PATHS
    ${USERVER_PG_LIBRARY_DIR}
    /usr/local/lib  # FreeBSD
)

_userver_module_find_library(
    NAMES libpgcommon.a
    PATHS
    ${USERVER_PG_SERVER_LIBRARY_DIR}
    ${USERVER_PG_LIBRARY_DIR}
    /usr/lib/postgresql/12/lib
    /usr/lib/postgresql/13/lib
    /usr/lib/postgresql/14/lib
    /usr/lib/postgresql/15/lib
    /usr/lib/postgresql/16/lib
    /usr/lib/postgresql/17/lib
    /usr/lib/postgresql/18/lib
    /usr/lib/postgresql/19/lib
    /usr/lib/postgresql/20/lib
)

_userver_module_find_library(
    NAMES libpgport.a
    PATHS
    ${USERVER_PG_SERVER_LIBRARY_DIR}
    ${USERVER_PG_LIBRARY_DIR}
    /usr/lib/postgresql/12/lib
    /usr/lib/postgresql/13/lib
    /usr/lib/postgresql/14/lib
    /usr/lib/postgresql/15/lib
    /usr/lib/postgresql/16/lib
    /usr/lib/postgresql/17/lib
    /usr/lib/postgresql/18/lib
    /usr/lib/postgresql/19/lib
    /usr/lib/postgresql/20/lib
)

_userver_module_end()
