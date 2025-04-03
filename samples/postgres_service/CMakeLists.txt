cmake_minimum_required(VERSION 3.14)
project(userver-samples-postgres_service CXX)

find_package(userver COMPONENTS postgresql REQUIRED)

add_executable(${PROJECT_NAME} "main.cpp")
target_link_libraries(${PROJECT_NAME} userver::postgresql)

# /// [Postgres service sample - CMakeLists.txt]
userver_add_sql_library(
    ${PROJECT_NAME}_sql
    NAMESPACE samples_postgres_service
    OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}
    SQL_FILES *.sql
)
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}_sql)
# /// [Postgres service sample - CMakeLists.txt]

userver_testsuite_add_simple()
