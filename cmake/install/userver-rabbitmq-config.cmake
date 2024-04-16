include_guard(GLOBAL)

if(userver_rabbitmq_FOUND)
  return()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/..")
include("${USERVER_CMAKE_DIR}/SetupAmqpCPP.cmake")

add_library(userver::rabbitmq ALIAS userver::userver-rabbitmq)

set(userver_rabbitmq_FOUND TRUE)
