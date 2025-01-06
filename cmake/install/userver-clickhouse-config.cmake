include_guard(GLOBAL)

if(userver_clickhouse_FOUND)
  return()
endif()

find_package(userver REQUIRED COMPONENTS
    core
)

include("${USERVER_CMAKE_DIR}/Findclickhouse-cpp.cmake")

set(userver_clickhouse_FOUND TRUE)
