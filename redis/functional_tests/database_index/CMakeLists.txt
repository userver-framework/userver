project(userver-redis-tests-database-index CXX)

add_executable(${PROJECT_NAME} "redis_service.cpp")
target_link_libraries(${PROJECT_NAME} userver::core userver::redis)

userver_chaos_testsuite_add()
