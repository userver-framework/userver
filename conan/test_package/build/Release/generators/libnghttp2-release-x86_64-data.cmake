########### AGGREGATED COMPONENTS AND DEPENDENCIES FOR THE MULTI CONFIG #####################
#############################################################################################

list(APPEND libnghttp2_COMPONENT_NAMES libnghttp2::nghttp2)
list(REMOVE_DUPLICATES libnghttp2_COMPONENT_NAMES)
set(libnghttp2_FIND_DEPENDENCY_NAMES "")

########### VARIABLES #######################################################################
#############################################################################################
set(libnghttp2_PACKAGE_FOLDER_RELEASE "/Users/alexiprof/.conan/data/libnghttp2/1.51.0/_/_/package/6841fe8f0f22f6fa260da36a43a94ab525c7ed8d")
set(libnghttp2_BUILD_MODULES_PATHS_RELEASE )


set(libnghttp2_INCLUDE_DIRS_RELEASE "${libnghttp2_PACKAGE_FOLDER_RELEASE}/include")
set(libnghttp2_RES_DIRS_RELEASE )
set(libnghttp2_DEFINITIONS_RELEASE )
set(libnghttp2_SHARED_LINK_FLAGS_RELEASE )
set(libnghttp2_EXE_LINK_FLAGS_RELEASE )
set(libnghttp2_OBJECTS_RELEASE )
set(libnghttp2_COMPILE_DEFINITIONS_RELEASE )
set(libnghttp2_COMPILE_OPTIONS_C_RELEASE )
set(libnghttp2_COMPILE_OPTIONS_CXX_RELEASE )
set(libnghttp2_LIB_DIRS_RELEASE "${libnghttp2_PACKAGE_FOLDER_RELEASE}/lib")
set(libnghttp2_LIBS_RELEASE nghttp2)
set(libnghttp2_SYSTEM_LIBS_RELEASE )
set(libnghttp2_FRAMEWORK_DIRS_RELEASE )
set(libnghttp2_FRAMEWORKS_RELEASE )
set(libnghttp2_BUILD_DIRS_RELEASE )

# COMPOUND VARIABLES
set(libnghttp2_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${libnghttp2_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${libnghttp2_COMPILE_OPTIONS_C_RELEASE}>")
set(libnghttp2_LINKER_FLAGS_RELEASE
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${libnghttp2_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${libnghttp2_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${libnghttp2_EXE_LINK_FLAGS_RELEASE}>")


set(libnghttp2_COMPONENTS_RELEASE libnghttp2::nghttp2)
########### COMPONENT libnghttp2::nghttp2 VARIABLES ############################################

set(libnghttp2_libnghttp2_nghttp2_INCLUDE_DIRS_RELEASE "${libnghttp2_PACKAGE_FOLDER_RELEASE}/include")
set(libnghttp2_libnghttp2_nghttp2_LIB_DIRS_RELEASE "${libnghttp2_PACKAGE_FOLDER_RELEASE}/lib")
set(libnghttp2_libnghttp2_nghttp2_RES_DIRS_RELEASE )
set(libnghttp2_libnghttp2_nghttp2_DEFINITIONS_RELEASE )
set(libnghttp2_libnghttp2_nghttp2_OBJECTS_RELEASE )
set(libnghttp2_libnghttp2_nghttp2_COMPILE_DEFINITIONS_RELEASE )
set(libnghttp2_libnghttp2_nghttp2_COMPILE_OPTIONS_C_RELEASE "")
set(libnghttp2_libnghttp2_nghttp2_COMPILE_OPTIONS_CXX_RELEASE "")
set(libnghttp2_libnghttp2_nghttp2_LIBS_RELEASE nghttp2)
set(libnghttp2_libnghttp2_nghttp2_SYSTEM_LIBS_RELEASE )
set(libnghttp2_libnghttp2_nghttp2_FRAMEWORK_DIRS_RELEASE )
set(libnghttp2_libnghttp2_nghttp2_FRAMEWORKS_RELEASE )
set(libnghttp2_libnghttp2_nghttp2_DEPENDENCIES_RELEASE )
set(libnghttp2_libnghttp2_nghttp2_SHARED_LINK_FLAGS_RELEASE )
set(libnghttp2_libnghttp2_nghttp2_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(libnghttp2_libnghttp2_nghttp2_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${libnghttp2_libnghttp2_nghttp2_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${libnghttp2_libnghttp2_nghttp2_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${libnghttp2_libnghttp2_nghttp2_EXE_LINK_FLAGS_RELEASE}>
)
set(libnghttp2_libnghttp2_nghttp2_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${libnghttp2_libnghttp2_nghttp2_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${libnghttp2_libnghttp2_nghttp2_COMPILE_OPTIONS_C_RELEASE}>")