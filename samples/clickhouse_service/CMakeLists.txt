project(userver-samples-clickhouse_service CXX)

add_executable(${PROJECT_NAME} "clickhouse_service.cpp")
target_link_libraries(${PROJECT_NAME} userver-core userver-clickhouse)

userver_sample_testsuite_add()
set_tests_properties(testsuite-${PROJECT_NAME} PROPERTIES ENVIRONMENT
        "TESTSUITE_CLICKHOUSE_SERVER_START_TIMEOUT=10.0")
