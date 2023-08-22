project(userver-samples-postgres_bitstring CXX)

add_executable(${PROJECT_NAME} "postgres_bitstring.cpp")
target_link_libraries(${PROJECT_NAME} userver-core userver-postgresql)

userver_sample_testsuite_add()
