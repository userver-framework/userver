project(userver-core-tests-basic-chaos CXX)

add_executable(${PROJECT_NAME} "service.cpp")
target_link_libraries(${PROJECT_NAME} userver-core)

userver_chaos_testsuite_add(TESTS_DIRECTORY tests)
userver_chaos_testsuite_add(TESTS_DIRECTORY tests-nonchaos)
userver_chaos_testsuite_add(TESTS_DIRECTORY tests-deadline)
userver_chaos_testsuite_add(TESTS_DIRECTORY tests-dynconf-cache)
