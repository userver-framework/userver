include_guard(GLOBAL)

if(userver_rocks_FOUND)
  return()
endif()

find_package(userver REQUIRED COMPONENTS
  core
)

find_package(RocksDB REQUIRED CONFIG)

set(userver_rocks_FOUND TRUE)
