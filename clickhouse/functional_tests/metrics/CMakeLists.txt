project(userver-clickhouse-tests-metrics CXX)

add_executable(${PROJECT_NAME} "service.cpp")
target_link_libraries(${PROJECT_NAME} userver-clickhouse)

userver_chaos_testsuite_add(
    ENV "TESTSUITE_CLICKHOUSE_SERVER_START_TIMEOUT=120.0"
)
