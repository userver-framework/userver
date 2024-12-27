include_guard(GLOBAL)

if(userver_clickhouse_FOUND)
  return()
endif()

find_package(userver REQUIRED COMPONENTS
    core
)

if (USERVER_CONAN)
  find_package(clickhouse-cpp REQUIRED CONFIG)
else()
  include("${USERVER_CMAKE_DIR}/modules/Findclickhouse-cpp.cmake")
endif()

set(userver_clickhouse_FOUND TRUE)
