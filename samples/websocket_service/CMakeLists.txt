project(userver-samples-websocket_service CXX)

add_executable(${PROJECT_NAME} "websocket_service.cpp")
target_link_libraries(${PROJECT_NAME} userver::core)

userver_sample_testsuite_add()
