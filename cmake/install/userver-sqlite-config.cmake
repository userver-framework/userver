include_guard(GLOBAL)

if(userver_sqlite_FOUND)
    return()
endif()

find_package(userver REQUIRED COMPONENTS core)

find_package(SQLite3 REQUIRED)

set(userver_sqlite_FOUND TRUE)
