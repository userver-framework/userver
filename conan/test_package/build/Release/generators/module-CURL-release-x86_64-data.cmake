########### AGGREGATED COMPONENTS AND DEPENDENCIES FOR THE MULTI CONFIG #####################
#############################################################################################

list(APPEND libcurl_COMPONENT_NAMES CURL::libcurl)
list(REMOVE_DUPLICATES libcurl_COMPONENT_NAMES)
list(APPEND libcurl_FIND_DEPENDENCY_NAMES ZLIB)
list(REMOVE_DUPLICATES libcurl_FIND_DEPENDENCY_NAMES)
set(ZLIB_FIND_MODE "MODULE")

########### VARIABLES #######################################################################
#############################################################################################
set(libcurl_PACKAGE_FOLDER_RELEASE "/Users/alexiprof/.conan/data/libcurl/7.86.0/_/_/package/a097455223234e250d01a2687cf7c15446fbd5d5")
set(libcurl_BUILD_MODULES_PATHS_RELEASE )


set(libcurl_INCLUDE_DIRS_RELEASE "${libcurl_PACKAGE_FOLDER_RELEASE}/include")
set(libcurl_RES_DIRS_RELEASE "${libcurl_PACKAGE_FOLDER_RELEASE}/res")
set(libcurl_DEFINITIONS_RELEASE "-DCURL_STATICLIB=1")
set(libcurl_SHARED_LINK_FLAGS_RELEASE )
set(libcurl_EXE_LINK_FLAGS_RELEASE )
set(libcurl_OBJECTS_RELEASE )
set(libcurl_COMPILE_DEFINITIONS_RELEASE "CURL_STATICLIB=1")
set(libcurl_COMPILE_OPTIONS_C_RELEASE )
set(libcurl_COMPILE_OPTIONS_CXX_RELEASE )
set(libcurl_LIB_DIRS_RELEASE "${libcurl_PACKAGE_FOLDER_RELEASE}/lib")
set(libcurl_LIBS_RELEASE curl)
set(libcurl_SYSTEM_LIBS_RELEASE )
set(libcurl_FRAMEWORK_DIRS_RELEASE )
set(libcurl_FRAMEWORKS_RELEASE CoreFoundation SystemConfiguration Security)
set(libcurl_BUILD_DIRS_RELEASE )

# COMPOUND VARIABLES
set(libcurl_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${libcurl_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${libcurl_COMPILE_OPTIONS_C_RELEASE}>")
set(libcurl_LINKER_FLAGS_RELEASE
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${libcurl_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${libcurl_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${libcurl_EXE_LINK_FLAGS_RELEASE}>")


set(libcurl_COMPONENTS_RELEASE CURL::libcurl)
########### COMPONENT CURL::libcurl VARIABLES ############################################

set(libcurl_CURL_libcurl_INCLUDE_DIRS_RELEASE "${libcurl_PACKAGE_FOLDER_RELEASE}/include")
set(libcurl_CURL_libcurl_LIB_DIRS_RELEASE "${libcurl_PACKAGE_FOLDER_RELEASE}/lib")
set(libcurl_CURL_libcurl_RES_DIRS_RELEASE "${libcurl_PACKAGE_FOLDER_RELEASE}/res")
set(libcurl_CURL_libcurl_DEFINITIONS_RELEASE "-DCURL_STATICLIB=1")
set(libcurl_CURL_libcurl_OBJECTS_RELEASE )
set(libcurl_CURL_libcurl_COMPILE_DEFINITIONS_RELEASE "CURL_STATICLIB=1")
set(libcurl_CURL_libcurl_COMPILE_OPTIONS_C_RELEASE "")
set(libcurl_CURL_libcurl_COMPILE_OPTIONS_CXX_RELEASE "")
set(libcurl_CURL_libcurl_LIBS_RELEASE curl)
set(libcurl_CURL_libcurl_SYSTEM_LIBS_RELEASE )
set(libcurl_CURL_libcurl_FRAMEWORK_DIRS_RELEASE )
set(libcurl_CURL_libcurl_FRAMEWORKS_RELEASE CoreFoundation SystemConfiguration Security)
set(libcurl_CURL_libcurl_DEPENDENCIES_RELEASE ZLIB::ZLIB)
set(libcurl_CURL_libcurl_SHARED_LINK_FLAGS_RELEASE )
set(libcurl_CURL_libcurl_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(libcurl_CURL_libcurl_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${libcurl_CURL_libcurl_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${libcurl_CURL_libcurl_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${libcurl_CURL_libcurl_EXE_LINK_FLAGS_RELEASE}>
)
set(libcurl_CURL_libcurl_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${libcurl_CURL_libcurl_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${libcurl_CURL_libcurl_COMPILE_OPTIONS_C_RELEASE}>")