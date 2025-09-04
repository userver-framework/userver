project(userver-core-tests-cache-update CXX)

add_executable(${PROJECT_NAME} "service.cpp")
target_link_libraries(${PROJECT_NAME} userver::core)

# Test that `userver_testsuite_add` works from sub directories.
add_subdirectory(tests)
