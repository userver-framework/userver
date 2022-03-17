project(userver-samples-redis_service CXX)

add_executable(${PROJECT_NAME} "redis_service.cpp")
target_link_libraries(${PROJECT_NAME} userver-core userver-redis)

userver_sample_testsuite_add()
