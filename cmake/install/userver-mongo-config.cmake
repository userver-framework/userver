include_guard(GLOBAL)

if(userver_mongo_FOUND)
  return()
endif()

find_package(userver REQUIRED COMPONENTS
    core
)

if (USERVER_CONAN)
  find_package(mongoc-1.0 REQUIRED CONFIG)
else()
  include("${USERVER_CMAKE_DIR}/SetupMongoDeps.cmake")
endif()

set(userver_mongo_FOUND TRUE)
