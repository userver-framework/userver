include_guard(GLOBAL)

if(userver_odbc_FOUND)
    return()
endif()

find_package(userver REQUIRED COMPONENTS core)

find_package(odbc REQUIRED)

set(userver_odbc_FOUND TRUE)
