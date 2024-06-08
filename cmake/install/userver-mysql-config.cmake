include_guard(GLOBAL)

if(userver_mysql_FOUND)
  return()
endif()

find_package(userver REQUIRED COMPONENTS
    core
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/..")
include("${USERVER_CMAKE_DIR}/Findlibmariadb.cmake")

add_library(userver::mysql ALIAS userver::userver-mysql)

set(userver_mysql_FOUND TRUE)
