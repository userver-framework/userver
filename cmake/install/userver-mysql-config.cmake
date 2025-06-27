include_guard(GLOBAL)

if(userver_mysql_FOUND)
    return()
endif()

find_package(userver REQUIRED COMPONENTS core)

find_package(libmariadb REQUIRED)

set(userver_mysql_FOUND TRUE)
