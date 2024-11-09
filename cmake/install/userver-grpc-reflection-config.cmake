include_guard(GLOBAL)

if(userver_grpc_reflection_FOUND)
  return()
endif()

find_package(userver REQUIRED COMPONENTS
    core grpc
)

set(userver_grpc_reflection_FOUND TRUE)


