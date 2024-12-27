include_guard(GLOBAL)

if(userver_mysql_FOUND)
  return()
endif()

find_package(userver REQUIRED COMPONENTS
    core
)

if (USERVER_CONAN)
  find_package(libmariadb REQUIRED CONFIG)
else()
  include("${USERVER_CMAKE_DIR}/modules/Findlibmariadb.cmake")
endif()

set(userver_mysql_FOUND TRUE)
