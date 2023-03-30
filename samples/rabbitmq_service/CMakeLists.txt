project(userver-samples-rabbitmq_service CXX)

add_executable(${PROJECT_NAME} "rabbitmq_service.cpp")
target_link_libraries(${PROJECT_NAME} userver-core userver-rabbitmq)

userver_sample_testsuite_add()
if (TEST testsuite-${PROJECT_NAME})
  set_tests_properties(testsuite-${PROJECT_NAME} PROPERTIES ENVIRONMENT
          "TESTSUITE_RABBITMQ_SERVER_START_TIMEOUT=60.0")
endif()
