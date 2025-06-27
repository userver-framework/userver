include_guard(GLOBAL)

if(userver_redis_FOUND)
    return()
endif()

find_package(userver REQUIRED COMPONENTS core)

find_package(hiredis REQUIRED)

set(userver_redis_FOUND TRUE)
