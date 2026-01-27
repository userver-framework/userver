include_guard(GLOBAL)

if(userver_s3api_FOUND)
    return()
endif()

find_package(userver REQUIRED COMPONENTS core)

find_package(pugixml REQUIRED)

set(userver_s3api_FOUND TRUE)
