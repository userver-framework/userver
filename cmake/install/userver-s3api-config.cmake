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
  find_package(Pugixml REQUIRED)
endif()

set(userver_s3api_FOUND TRUE)

