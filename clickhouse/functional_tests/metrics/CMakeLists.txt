project(userver-clickhouse-tests-metrics CXX)

add_executable(${PROJECT_NAME} "service.cpp")
target_link_libraries(${PROJECT_NAME} userver-clickhouse)

userver_chaos_testsuite_add()
if (TEST testsuite-${PROJECT_NAME})
  set_tests_properties(testsuite-${PROJECT_NAME} PROPERTIES ENVIRONMENT
          "TESTSUITE_CLICKHOUSE_SERVER_START_TIMEOUT=30.0")
endif()
