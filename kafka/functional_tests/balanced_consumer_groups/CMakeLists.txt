project(userver-kafka-tests-balanced-consumer-groups CXX)

add_executable(${PROJECT_NAME} "kafka_service.cpp")
target_link_libraries(${PROJECT_NAME} userver::core userver::kafka)

userver_chaos_testsuite_add(
  ENV
  "TESTSUITE_KAFKA_SERVER_START_TIMEOUT=120.0"
  "TESTSUITE_KAFKA_SERVER_HOST=[::1]"
  "TESTSUITE_KAFKA_SERVER_PORT=8099"
  "TESTSUITE_KAFKA_CONTROLLER_PORT=8100"
  "TESTSUITE_KAFKA_CUSTOM_TOPICS=test-topic:2")
