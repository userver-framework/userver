include_guard(GLOBAL)

if(userver_chaotic_openapi_FOUND)
    return()
endif()

find_package(userver REQUIRED COMPONENTS core chaotic)

set(userver_chaotic_openapi_FOUND TRUE)
