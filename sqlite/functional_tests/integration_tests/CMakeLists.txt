project(userver-sqlite-tests-basic CXX)

add_executable(${PROJECT_NAME} "service.cpp")
target_link_libraries(${PROJECT_NAME} userver-core userver-sqlite)

userver_chaos_testsuite_add(TESTS_DIRECTORY tests)
