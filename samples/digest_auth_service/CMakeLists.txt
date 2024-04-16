project(userver-samples-digest_auth CXX)

add_executable(${PROJECT_NAME} digest_auth_service.cpp auth_digest.cpp)
target_link_libraries(${PROJECT_NAME} userver::core userver::postgresql)

userver_sample_testsuite_add()

set_tests_properties(testsuite-${PROJECT_NAME} PROPERTIES ENVIRONMENT
          "SERVER_DIGEST_AUTH_SECRET={\"http_server_digest_auth_secret\": \"some-private-key\"}")
