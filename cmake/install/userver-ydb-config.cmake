if(USERVER_YDB_CONFIG_PHASE STREQUAL "deps")
    if(_USERVER_YDB_DEPS_LOADED)
        return()
    endif()

    find_dependency(googleapis CONFIG)
    find_dependency(ydb-cpp-sdk CONFIG)

    set(_USERVER_YDB_DEPS_LOADED TRUE)
    return()
endif()

if(userver_ydb_FOUND)
    return()
endif()

find_package(userver REQUIRED COMPONENTS core)

include("${USERVER_CMAKE_DIR}/UserverSql.cmake")

set(userver_ydb_FOUND TRUE)
