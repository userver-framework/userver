project(userver-kafka-tests-integrational-tests CXX)

add_executable(${PROJECT_NAME} "kafka_service.cpp")
target_link_libraries(${PROJECT_NAME} userver::core userver::kafka)

userver_chaos_testsuite_add(
    ENV "TESTSUITE_KAFKA_CUSTOM_TOPICS=test-topic-send:1,test-topic-consume-1:1,test-topic-consume-2:1,test-topic-consume-produced-1:1,test-topic-consume-produced-2:1" "TESTSUITE_KAFKA_SERVER_PORT=8088"
)
