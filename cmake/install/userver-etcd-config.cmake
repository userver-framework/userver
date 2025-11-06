include_guard(GLOBAL)

if(userver_etcd_FOUND)
  return()
endif()

find_package(userver REQUIRED COMPONENTS
  core
)

set(userver_etcd_FOUND TRUE)
