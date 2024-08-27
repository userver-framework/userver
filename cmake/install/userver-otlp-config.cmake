include_guard(GLOBAL)

if(userver_otlp_FOUND)
  return()
endif()

find_package(userver REQUIRED COMPONENTS
    core
)

set(userver_otlp_FOUND TRUE)
