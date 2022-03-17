project(userver-samples-hello_service CXX)

add_executable(${PROJECT_NAME} "hello_service.cpp")
target_link_libraries(${PROJECT_NAME} userver-core)

userver_sample_testsuite_add()
