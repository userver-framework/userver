# TODO: not working yet
include_guard(GLOBAL)

if(userver_ydb_FOUND)
    return()
endif()

find_package(userver REQUIRED COMPONENTS core)

# include("${USERVER_CMAKE_DIR}/ydb-cpp-sdk.cmake")

include("${USERVER_CMAKE_DIR}/UserverSql.cmake")

set(userver_ydb_FOUND TRUE)
