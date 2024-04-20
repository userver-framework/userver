include_guard(GLOBAL)

if(userver_mongo_FOUND)
  return()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/..")
include("${USERVER_CMAKE_DIR}/Findbson.cmake")
include("${USERVER_CMAKE_DIR}/Findmongoc.cmake")

add_library(userver::mongo ALIAS userver::userver-mongo)

set(userver_mongo_FOUND TRUE)
