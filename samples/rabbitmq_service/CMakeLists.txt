project(userver-samples-rabbitmq_service CXX)

add_executable(${PROJECT_NAME} "rabbitmq_service.cpp")
target_link_libraries(${PROJECT_NAME} userver-core userver-rabbitmq)

userver_sample_testsuite_add()
