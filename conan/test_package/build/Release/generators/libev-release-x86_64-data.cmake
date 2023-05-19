########### AGGREGATED COMPONENTS AND DEPENDENCIES FOR THE MULTI CONFIG #####################
#############################################################################################

set(libev_COMPONENT_NAMES "")
set(libev_FIND_DEPENDENCY_NAMES "")

########### VARIABLES #######################################################################
#############################################################################################
set(libev_PACKAGE_FOLDER_RELEASE "/Users/alexiprof/.conan/data/libev/4.33/_/_/package/6841fe8f0f22f6fa260da36a43a94ab525c7ed8d")
set(libev_BUILD_MODULES_PATHS_RELEASE )


set(libev_INCLUDE_DIRS_RELEASE "${libev_PACKAGE_FOLDER_RELEASE}/include")
set(libev_RES_DIRS_RELEASE )
set(libev_DEFINITIONS_RELEASE )
set(libev_SHARED_LINK_FLAGS_RELEASE )
set(libev_EXE_LINK_FLAGS_RELEASE )
set(libev_OBJECTS_RELEASE )
set(libev_COMPILE_DEFINITIONS_RELEASE )
set(libev_COMPILE_OPTIONS_C_RELEASE )
set(libev_COMPILE_OPTIONS_CXX_RELEASE )
set(libev_LIB_DIRS_RELEASE "${libev_PACKAGE_FOLDER_RELEASE}/lib")
set(libev_LIBS_RELEASE ev)
set(libev_SYSTEM_LIBS_RELEASE )
set(libev_FRAMEWORK_DIRS_RELEASE )
set(libev_FRAMEWORKS_RELEASE )
set(libev_BUILD_DIRS_RELEASE "${libev_PACKAGE_FOLDER_RELEASE}/")

# COMPOUND VARIABLES
set(libev_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${libev_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${libev_COMPILE_OPTIONS_C_RELEASE}>")
set(libev_LINKER_FLAGS_RELEASE
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${libev_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${libev_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${libev_EXE_LINK_FLAGS_RELEASE}>")


set(libev_COMPONENTS_RELEASE )