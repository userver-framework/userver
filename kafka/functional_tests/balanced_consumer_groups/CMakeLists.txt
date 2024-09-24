project(userver-kafka-tests-balanced-consumer-groups CXX)

add_executable(${PROJECT_NAME} "kafka_service.cpp")
target_link_libraries(${PROJECT_NAME} userver::core userver::kafka)

userver_chaos_testsuite_add(
    ENV "KAFKA_RECIPE_BROKER_LIST=localhost:7777"
)
