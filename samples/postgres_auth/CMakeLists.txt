project(userver-samples-postgres_auth CXX)

add_executable(${PROJECT_NAME} postgres_service.cpp auth_bearer.cpp)
target_link_libraries(${PROJECT_NAME} userver-core userver-postgresql)

userver_sample_testsuite_add()
