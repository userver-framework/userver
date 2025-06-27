project(userver-odbc CXX)

userver_module(
    odbc
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
    LINK_LIBRARIES_PRIVATE odbc
    DBTEST_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/tests"
    UTEST_LINK_LIBRARIES odbc
    DBTEST_DATABASES postgresql
    DBTEST_ENV POSTGRES_TEST_DSN=postgresql://testsuite@localhost:15433/postgres
)

_userver_directory_install(
    COMPONENT odbc
    FILES "${USERVER_ROOT_DIR}/cmake/modules/Findlibodbc.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/userver/modules"
)
