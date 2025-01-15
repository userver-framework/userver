include_guard(GLOBAL)

if(userver_redis_FOUND)
  return()
endif()

find_package(userver REQUIRED COMPONENTS
    core
)

if (USERVER_CONAN)
  find_package(hiredis REQUIRED CONFIG)
else()
  find_package(Hiredis REQUIRED)
endif()

set(userver_redis_FOUND TRUE)
