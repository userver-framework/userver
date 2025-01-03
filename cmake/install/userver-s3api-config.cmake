include_guard(GLOBAL)

if(userver_s3api_FOUND)
  return()
endif()

find_package(userver REQUIRED COMPONENTS
    core
)

if(USERVER_CONAN)
  find_package(pugixml REQUIRED CONFIG)
else()
  include("${USERVER_CMAKE_DIR}/modules/FindPugixml.cmake")
endif()

set(userver_s3api_FOUND TRUE)

