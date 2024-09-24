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
  TEST_ENV "TESTSUITE_KAFKA_CUSTOM_TOPICS=large-topic-1:4,large-topic-2:4"
)
