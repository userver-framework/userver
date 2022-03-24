project(userver-samples-testsuite-support CXX)

add_executable(${PROJECT_NAME} main.cpp now.cpp testpoint.cpp logcapture.cpp)
target_link_libraries(${PROJECT_NAME} userver-core)

userver_sample_testsuite_add()
