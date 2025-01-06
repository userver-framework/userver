include_guard(GLOBAL)

if(userver_mongo_FOUND)
  return()
endif()

find_package(userver REQUIRED COMPONENTS
    core
)

include("${USERVER_CMAKE_DIR}/modules/Findbson.cmake")
include("${USERVER_CMAKE_DIR}/modules/Findmongoc.cmake")

set(userver_mongo_FOUND TRUE)
