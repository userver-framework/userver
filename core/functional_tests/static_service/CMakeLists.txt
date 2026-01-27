project(userver-core-tests-static-service CXX)

add_executable(${PROJECT_NAME} "main.cpp")
target_link_libraries(${PROJECT_NAME} userver::core)

userver_testsuite_add_simple(WORKING_DIRECTORY tests-serve-from-root)
userver_testsuite_add_simple(WORKING_DIRECTORY tests-serve-from-subpath)
