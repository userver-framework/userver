include_guard(GLOBAL)

if(userver_redis_FOUND)
  return()
endif()

find_package(userver REQUIRED COMPONENTS
    core
)

include("${USERVER_CMAKE_DIR}/FindHiredis.cmake")

set(userver_redis_FOUND TRUE)
