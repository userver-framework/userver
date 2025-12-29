project(userver-core-tests-early-monitor-port-open CXX)

file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*pp")
add_executable(${PROJECT_NAME} ${SOURCES} "main.cpp")
target_link_libraries(${PROJECT_NAME} userver::core)

userver_chaos_testsuite_add(TESTS_DIRECTORY tests)
userver_chaos_testsuite_add(TESTS_DIRECTORY tests-fail-during-boot)
