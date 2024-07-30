include_guard(GLOBAL)

if(userver_mysql_FOUND)
  return()
endif()

find_package(userver REQUIRED COMPONENTS
    core
)

include("${USERVER_CMAKE_DIR}/Findlibmariadb.cmake")

set(userver_mysql_FOUND TRUE)
