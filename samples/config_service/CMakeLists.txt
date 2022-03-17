project(userver-samples-config_service CXX)

add_executable(${PROJECT_NAME} "config_service.cpp")
target_link_libraries(${PROJECT_NAME} userver-core)

userver_sample_testsuite_add()
