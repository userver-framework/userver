project(userver-samples-production_service CXX)

add_executable(${PROJECT_NAME} "production_service.cpp")
target_link_libraries(${PROJECT_NAME} userver-core)

userver_sample_testsuite_add()
