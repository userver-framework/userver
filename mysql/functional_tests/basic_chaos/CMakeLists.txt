project(userver-mysql-tests-basic-chaos CXX)

add_executable(${PROJECT_NAME} "mysql_service.cpp")
target_link_libraries(${PROJECT_NAME} userver-mysql mariadb)

userver_chaos_testsuite_add(
    ENV "TESTSUITE_MYSQL_SERVER_START_TIMEOUT=120.0"
)
