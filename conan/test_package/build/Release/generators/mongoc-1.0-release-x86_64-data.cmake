########### AGGREGATED COMPONENTS AND DEPENDENCIES FOR THE MULTI CONFIG #####################
#############################################################################################

list(APPEND mongo-c-driver_COMPONENT_NAMES mongo::bson_static mongo::mongoc_static)
list(REMOVE_DUPLICATES mongo-c-driver_COMPONENT_NAMES)
list(APPEND mongo-c-driver_FIND_DEPENDENCY_NAMES OpenSSL cyrus-sasl Snappy ZLIB zstd ICU)
list(REMOVE_DUPLICATES mongo-c-driver_FIND_DEPENDENCY_NAMES)
set(OpenSSL_FIND_MODE "NO_MODULE")
set(cyrus-sasl_FIND_MODE "NO_MODULE")
set(Snappy_FIND_MODE "NO_MODULE")
set(ZLIB_FIND_MODE "NO_MODULE")
set(zstd_FIND_MODE "NO_MODULE")
set(ICU_FIND_MODE "NO_MODULE")

########### VARIABLES #######################################################################
#############################################################################################
set(mongo-c-driver_PACKAGE_FOLDER_RELEASE "/Users/alexiprof/.conan/data/mongo-c-driver/1.22.0/_/_/package/122c7fdd6ac5f79d4d69de394058fec92c456b93")
set(mongo-c-driver_BUILD_MODULES_PATHS_RELEASE )


set(mongo-c-driver_INCLUDE_DIRS_RELEASE "${mongo-c-driver_PACKAGE_FOLDER_RELEASE}/include/libmongoc-1.0"
			"${mongo-c-driver_PACKAGE_FOLDER_RELEASE}/include/libbson-1.0")
set(mongo-c-driver_RES_DIRS_RELEASE )
set(mongo-c-driver_DEFINITIONS_RELEASE "-DMONGOC_STATIC"
			"-DBSON_STATIC")
set(mongo-c-driver_SHARED_LINK_FLAGS_RELEASE )
set(mongo-c-driver_EXE_LINK_FLAGS_RELEASE )
set(mongo-c-driver_OBJECTS_RELEASE )
set(mongo-c-driver_COMPILE_DEFINITIONS_RELEASE "MONGOC_STATIC"
			"BSON_STATIC")
set(mongo-c-driver_COMPILE_OPTIONS_C_RELEASE )
set(mongo-c-driver_COMPILE_OPTIONS_CXX_RELEASE )
set(mongo-c-driver_LIB_DIRS_RELEASE "${mongo-c-driver_PACKAGE_FOLDER_RELEASE}/lib")
set(mongo-c-driver_LIBS_RELEASE mongoc-static-1.0 bson-static-1.0)
set(mongo-c-driver_SYSTEM_LIBS_RELEASE resolv)
set(mongo-c-driver_FRAMEWORK_DIRS_RELEASE )
set(mongo-c-driver_FRAMEWORKS_RELEASE )
set(mongo-c-driver_BUILD_DIRS_RELEASE )

# COMPOUND VARIABLES
set(mongo-c-driver_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${mongo-c-driver_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${mongo-c-driver_COMPILE_OPTIONS_C_RELEASE}>")
set(mongo-c-driver_LINKER_FLAGS_RELEASE
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${mongo-c-driver_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${mongo-c-driver_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${mongo-c-driver_EXE_LINK_FLAGS_RELEASE}>")


set(mongo-c-driver_COMPONENTS_RELEASE mongo::bson_static mongo::mongoc_static)
########### COMPONENT mongo::mongoc_static VARIABLES ############################################

set(mongo-c-driver_mongo_mongoc_static_INCLUDE_DIRS_RELEASE "${mongo-c-driver_PACKAGE_FOLDER_RELEASE}/include/libmongoc-1.0")
set(mongo-c-driver_mongo_mongoc_static_LIB_DIRS_RELEASE "${mongo-c-driver_PACKAGE_FOLDER_RELEASE}/lib")
set(mongo-c-driver_mongo_mongoc_static_RES_DIRS_RELEASE )
set(mongo-c-driver_mongo_mongoc_static_DEFINITIONS_RELEASE "-DMONGOC_STATIC")
set(mongo-c-driver_mongo_mongoc_static_OBJECTS_RELEASE )
set(mongo-c-driver_mongo_mongoc_static_COMPILE_DEFINITIONS_RELEASE "MONGOC_STATIC")
set(mongo-c-driver_mongo_mongoc_static_COMPILE_OPTIONS_C_RELEASE "")
set(mongo-c-driver_mongo_mongoc_static_COMPILE_OPTIONS_CXX_RELEASE "")
set(mongo-c-driver_mongo_mongoc_static_LIBS_RELEASE mongoc-static-1.0)
set(mongo-c-driver_mongo_mongoc_static_SYSTEM_LIBS_RELEASE resolv)
set(mongo-c-driver_mongo_mongoc_static_FRAMEWORK_DIRS_RELEASE )
set(mongo-c-driver_mongo_mongoc_static_FRAMEWORKS_RELEASE )
set(mongo-c-driver_mongo_mongoc_static_DEPENDENCIES_RELEASE mongo::bson_static openssl::openssl cyrus-sasl::cyrus-sasl Snappy::snappy ZLIB::ZLIB zstd::libzstd_static icu::icu)
set(mongo-c-driver_mongo_mongoc_static_SHARED_LINK_FLAGS_RELEASE )
set(mongo-c-driver_mongo_mongoc_static_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(mongo-c-driver_mongo_mongoc_static_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${mongo-c-driver_mongo_mongoc_static_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${mongo-c-driver_mongo_mongoc_static_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${mongo-c-driver_mongo_mongoc_static_EXE_LINK_FLAGS_RELEASE}>
)
set(mongo-c-driver_mongo_mongoc_static_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${mongo-c-driver_mongo_mongoc_static_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${mongo-c-driver_mongo_mongoc_static_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT mongo::bson_static VARIABLES ############################################

set(mongo-c-driver_mongo_bson_static_INCLUDE_DIRS_RELEASE "${mongo-c-driver_PACKAGE_FOLDER_RELEASE}/include/libbson-1.0")
set(mongo-c-driver_mongo_bson_static_LIB_DIRS_RELEASE "${mongo-c-driver_PACKAGE_FOLDER_RELEASE}/lib")
set(mongo-c-driver_mongo_bson_static_RES_DIRS_RELEASE )
set(mongo-c-driver_mongo_bson_static_DEFINITIONS_RELEASE "-DBSON_STATIC")
set(mongo-c-driver_mongo_bson_static_OBJECTS_RELEASE )
set(mongo-c-driver_mongo_bson_static_COMPILE_DEFINITIONS_RELEASE "BSON_STATIC")
set(mongo-c-driver_mongo_bson_static_COMPILE_OPTIONS_C_RELEASE "")
set(mongo-c-driver_mongo_bson_static_COMPILE_OPTIONS_CXX_RELEASE "")
set(mongo-c-driver_mongo_bson_static_LIBS_RELEASE bson-static-1.0)
set(mongo-c-driver_mongo_bson_static_SYSTEM_LIBS_RELEASE )
set(mongo-c-driver_mongo_bson_static_FRAMEWORK_DIRS_RELEASE )
set(mongo-c-driver_mongo_bson_static_FRAMEWORKS_RELEASE )
set(mongo-c-driver_mongo_bson_static_DEPENDENCIES_RELEASE )
set(mongo-c-driver_mongo_bson_static_SHARED_LINK_FLAGS_RELEASE )
set(mongo-c-driver_mongo_bson_static_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(mongo-c-driver_mongo_bson_static_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${mongo-c-driver_mongo_bson_static_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${mongo-c-driver_mongo_bson_static_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${mongo-c-driver_mongo_bson_static_EXE_LINK_FLAGS_RELEASE}>
)
set(mongo-c-driver_mongo_bson_static_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${mongo-c-driver_mongo_bson_static_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${mongo-c-driver_mongo_bson_static_COMPILE_OPTIONS_C_RELEASE}>")