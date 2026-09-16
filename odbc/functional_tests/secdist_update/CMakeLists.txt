project(userver-odbc-tests-secdist-update CXX)

add_executable(${PROJECT_NAME} "odbc_service.cpp")
target_link_libraries(${PROJECT_NAME} userver::odbc)

userver_chaos_testsuite_add(
    ENV "TESTSUITE_PGSQL_SERVER_START_TIMEOUT=120.0"
    RESOURCE_LOCKS userver_postgresql
)
