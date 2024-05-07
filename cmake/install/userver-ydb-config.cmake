# TODO: not working yet
include_guard(GLOBAL)

if(userver_ydb_FOUND)
  return()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/..")
# include("${USERVER_CMAKE_DIR}/ydb-cpp-sdk.cmake")

add_library(userver::ydb ALIAS userver::userver-ydb)

set(userver_ydb_FOUND TRUE)
