project(userver-samples-mysql_service CXX)

add_executable(${PROJECT_NAME} "mysql_service.cpp")
target_link_libraries(${PROJECT_NAME} userver-mysql)
target_link_libraries(${PROJECT_NAME} mariadbclient)

userver_sample_testsuite_add()
