include_guard(GLOBAL)

if(userver_clickhouse_FOUND)
  return()
endif()

find_package(userver REQUIRED COMPONENTS
    core
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/..")
include("${USERVER_CMAKE_DIR}/Findclickhouse-cpp.cmake")

add_library(userver::clickhouse ALIAS userver::userver-clickhouse)

set(userver_clickhouse_FOUND TRUE)
