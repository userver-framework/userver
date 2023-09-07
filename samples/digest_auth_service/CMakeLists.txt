project(userver-samples-digest_auth CXX)

add_executable(${PROJECT_NAME} digest_auth_service.cpp auth_digest.cpp)
target_link_libraries(${PROJECT_NAME} userver-core userver-postgresql)

userver_sample_testsuite_add()

set_tests_properties(testsuite-${PROJECT_NAME} PROPERTIES ENVIRONMENT
          "DIGEST_SERVER_SECRET={\"server-secret-key\": \"some-private-key\"}")
