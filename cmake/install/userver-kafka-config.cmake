include_guard(GLOBAL)

if(userver_kafka_FOUND)
  return()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/..")
include("${USERVER_CMAKE_DIR}/FindRdKafka.cmake")

add_library(userver::kafka ALIAS userver::userver-kafka)

set(userver_kafka_FOUND TRUE)
