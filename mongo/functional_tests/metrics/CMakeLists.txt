project(userver-mongo-tests-metrics CXX)

add_executable(${PROJECT_NAME} "mongo_service.cpp")
target_link_libraries(${PROJECT_NAME} userver-mongo)

userver_chaos_testsuite_add(TESTS_DIRECTORY tests)
userver_chaos_testsuite_add(TESTS_DIRECTORY tests-values)
