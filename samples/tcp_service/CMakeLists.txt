project(userver-samples-tcp_service CXX)

add_executable(${PROJECT_NAME} "tcp_service.cpp")
target_link_libraries(${PROJECT_NAME} userver::core)

userver_sample_testsuite_add()
