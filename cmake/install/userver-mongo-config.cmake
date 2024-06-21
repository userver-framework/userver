include_guard(GLOBAL)

if(userver_mongo_FOUND)
  return()
endif()

find_package(userver REQUIRED COMPONENTS
    core
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/..")
include("${USERVER_CMAKE_DIR}/Findbson.cmake")
include("${USERVER_CMAKE_DIR}/Findmongoc.cmake")

set(userver_mongo_FOUND TRUE)
