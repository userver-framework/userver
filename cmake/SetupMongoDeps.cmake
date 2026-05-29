include_guard(GLOBAL)

option(USERVER_MONGODB_USE_CMAKE_CONFIG "Use mongoc cmake configuration" ON)

if(USERVER_MONGODB_USE_CMAKE_CONFIG)
    # Try to find via modern mongo-c-driver 2.x.x way first
    find_package(mongoc 2.0.0 QUIET CONFIG)
    if(mongoc_FOUND)
        message(STATUS "Mongoc: using config version: (bson: ${bson_VERSION}, mongoc: ${mongoc_VERSION})")

        # prefer static libs here for compat
        # https://github.com/mongodb/mongo-c-driver/blob/2.0.0/src/libmongoc/etc/mongocConfig.cmake.in#L7-L12
        # mongoc::mongoc is mongoc::static first then mongoc::shared. Same for bson.
        set(USERVER_MONGO_LIBMONGOC_NAME mongoc::mongoc)
        set(USERVER_MONGO_LIBBSON_NAME bson::bson)

        return()
    endif()

    # Go back to 1.x.x
    find_package(mongoc-1.0 QUIET CONFIG)
    if(mongoc-1.0_FOUND)
        message(STATUS "Mongoc: using config version: (bson: ${bson-1.0_VERSION}, mongoc: ${mongoc-1.0_VERSION})")
        return()
    endif()
endif()

find_package(bson REQUIRED)
find_package(mongoc REQUIRED)
