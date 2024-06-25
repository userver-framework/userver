include_guard(GLOBAL)

if(userver_chaotic_FOUND)
  return()
endif()

find_package(userver REQUIRED COMPONENTS
    core
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/..")

set_property(GLOBAL PROPERTY userver_chaotic_extra_args "-I ${CMAKE_CURRENT_LIST_DIR}/../../../include")
set(USERVER_CHAOTIC_SCRIPTS_PATH "${USERVER_CMAKE_DIR}/chaotic")
include(ChaoticGen)

set(userver_chaotic_FOUND TRUE)
