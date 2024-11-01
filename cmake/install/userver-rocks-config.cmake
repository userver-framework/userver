include_guard(GLOBAL)

if(userver_rocks_FOUND)
  return()
endif()

find_package(userver REQUIRED COMPONENTS
  core
)

include("${USERVER_CMAKE_DIR}/FindRocksDB.cmake")

set(userver_rocks_FOUND TRUE)
