include_guard(GLOBAL)

if(userver_s3api_FOUND)
  return()
endif()

find_package(userver REQUIRED COMPONENTS
    core
)

include("${USERVER_CMAKE_DIR}/modules/FindPugixml.cmake")

set(userver_s3api_FOUND TRUE)

