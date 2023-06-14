project(userver-samples-clickhouse_service CXX)

add_executable(${PROJECT_NAME} "clickhouse_service.cpp")
target_link_libraries(${PROJECT_NAME} userver-core userver-clickhouse)

userver_sample_testsuite_add()
if (TEST testsuite-${PROJECT_NAME})
  set_tests_properties(testsuite-${PROJECT_NAME} PROPERTIES ENVIRONMENT
          "TESTSUITE_CLICKHOUSE_SERVER_START_TIMEOUT=30.0")
endif()
