project(userver-odbc-tests-basic-chaos CXX)

add_executable(${PROJECT_NAME} "odbc_service.cpp")
target_link_libraries(${PROJECT_NAME} userver::odbc)

userver_chaos_testsuite_add(
    ENV "TESTSUITE_PGSQL_SERVER_START_TIMEOUT=120.0"
        # The odbc service talks to postgresql without linking userver::postgresql, so the
        # shared lock must be declared explicitly.
    RESOURCE_LOCKS userver_postgresql
)
