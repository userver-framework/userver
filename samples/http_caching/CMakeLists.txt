project(userver-samples-http_caching CXX)

add_executable(${PROJECT_NAME} "http_caching.cpp")
target_link_libraries(${PROJECT_NAME} userver-core)

userver_sample_testsuite_add()
