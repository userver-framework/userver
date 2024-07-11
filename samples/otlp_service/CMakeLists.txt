project(userver-samples-otlp_service CXX)

add_executable(${PROJECT_NAME} "otlp_service.cpp")
target_link_libraries(${PROJECT_NAME} userver::otlp)

userver_testsuite_add_simple()
