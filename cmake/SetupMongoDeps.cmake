include_guard(GLOBAL)

option(USERVER_MONGODB_USE_CMAKE_CONFIG "Use mongoc cmake configuration" ON)

if(USERVER_MONGODB_USE_CMAKE_CONFIG)
    find_package(mongoc-1.0 QUIET CONFIG)
    if(mongoc-1.0_FOUND)
        message(STATUS "Mongoc: using config version: (bson: ${bson-1.0_VERSION}, mongoc: ${mongoc-1.0_VERSION})")
        return()
    endif()
endif()

find_package(bson REQUIRED)
find_package(mongoc REQUIRED)
