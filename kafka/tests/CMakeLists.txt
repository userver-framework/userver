project(userver-kafka-kafkatest)

file(GLOB_RECURSE KAFKA_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)

add_executable(${PROJECT_NAME} ${KAFKA_TEST_SOURCES})
target_link_libraries(${PROJECT_NAME}
  PRIVATE userver::kafka userver::utest
)
target_include_directories(${PROJECT_NAME}
  PRIVATE $<TARGET_PROPERTY:userver::kafka,INCLUDE_DIRECTORIES>
)
userver_add_utest(
  NAME ${PROJECT_NAME}
  DATABASES kafka
  TEST_ENV
  "TESTSUITE_KAFKA_SERVER_START_TIMEOUT=120.0"
  "TESTSUITE_KAFKA_SERVER_HOST=[::1]"
  "TESTSUITE_KAFKA_SERVER_PORT=8099"
  "TESTSUITE_KAFKA_CONTROLLER_PORT=8100"
  "TESTSUITE_KAFKA_CUSTOM_TOPICS=lt-1:4,lt-2:4,tt-1:1,tt-2:1,tt-3:1,tt-4:1,tt-5:1,tt-6:1,tt-7:1,tt-8:1"
)
