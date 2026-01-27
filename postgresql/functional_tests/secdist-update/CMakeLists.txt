project(userver-postgresql-tests-secdist-update CXX)

add_executable(${PROJECT_NAME} "postgres_service.cpp")
target_link_libraries(${PROJECT_NAME} userver::core userver::postgresql)

userver_chaos_testsuite_add(TESTS_DIRECTORY tests)
