include_guard(GLOBAL)

if(userver_rocks_FOUND)
  return()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/..")
include("${USERVER_CMAKE_DIR}/SetupRocksDeps.cmake")

add_library(userver::rocks ALIAS userver::userver-rocks)

set(userver_rocks_FOUND TRUE)
