project(userver-samples-static_service CXX)

add_executable(${PROJECT_NAME} "static_service.cpp")
target_link_libraries(${PROJECT_NAME} userver-core)

userver_sample_testsuite_add()
