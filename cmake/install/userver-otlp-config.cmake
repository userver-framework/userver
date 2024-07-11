include_guard(GLOBAL)

if(userver_otlp_FOUND)
  return()
endif()

find_package(userver REQUIRED COMPONENTS
    core
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/..")

set(userver_otlp_FOUND TRUE)
