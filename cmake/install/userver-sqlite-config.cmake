include_guard(GLOBAL)

if(userver_sqlite_FOUND)
  return()
endif()

find_package(userver REQUIRED COMPONENTS
  core
)

if(USERVER_CONAN)
  find_package(SQLite3 REQUIRED)
else()
  find_package(SQLite3 REQUIRED)
endif()

set(userver_sqlite_FOUND TRUE)
