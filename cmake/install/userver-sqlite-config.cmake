include_guard(GLOBAL)

if(userver_sqlite3_FOUND)
  return()
endif()

find_package(userver REQUIRED COMPONENTS
    core
)

include(${USERVER_CMAKE_DIR}/FindSQLite.cmake)

set(userver_sqlite3_FOUND TRUE)
