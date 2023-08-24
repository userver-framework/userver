project(userver-rabbitmq-tests-metrics CXX)

add_executable(${PROJECT_NAME} "rabbitmq_service.cpp")
target_link_libraries(${PROJECT_NAME} userver-rabbitmq)

userver_chaos_testsuite_add()
