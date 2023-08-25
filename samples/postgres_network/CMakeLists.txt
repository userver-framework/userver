project(userver-samples-postgres_network CXX)

add_executable(${PROJECT_NAME} "postgres_network.cpp")
target_link_libraries(${PROJECT_NAME} userver-core userver-universal userver-postgresql)

userver_sample_testsuite_add()
