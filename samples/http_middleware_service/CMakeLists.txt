project(userver-samples-http_middleware_service CXX)

add_executable(${PROJECT_NAME} "http_middleware_service.cpp")
target_link_libraries(${PROJECT_NAME} userver::core)

userver_sample_testsuite_add()
