include_guard(GLOBAL)

if(userver_lib_s3api_FOUND)
  return()
endif()

find_package(userver REQUIRED COMPONENTS
    core
)

include("${USERVER_CMAKE_DIR}/FindPugixml.cmake")

set(userver_lib_s3api_FOUND TRUE)

