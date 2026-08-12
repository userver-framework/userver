include_guard(GLOBAL)

if(userver_clickhouse_FOUND)
    return()
endif()

find_package(userver REQUIRED COMPONENTS core)

find_package(clickhouse-cpp REQUIRED)

set(userver_clickhouse_FOUND TRUE)
