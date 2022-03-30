project(userver-samples-clickhouse_service CXX)

add_executable(${PROJECT_NAME} "clickhouse_service.cpp")
target_link_libraries(${PROJECT_NAME} userver-core userver-clickhouse)

userver_sample_testsuite_add()
