########### AGGREGATED COMPONENTS AND DEPENDENCIES FOR THE MULTI CONFIG #####################
#############################################################################################

set(bzip2_COMPONENT_NAMES "")
set(bzip2_FIND_DEPENDENCY_NAMES "")

########### VARIABLES #######################################################################
#############################################################################################
set(bzip2_PACKAGE_FOLDER_RELEASE "/Users/alexiprof/.conan/data/bzip2/1.0.8/_/_/package/4d994402bb6d6e8babdc89ce439cd1a0ab9877cd")
set(bzip2_BUILD_MODULES_PATHS_RELEASE "${bzip2_PACKAGE_FOLDER_RELEASE}/lib/cmake/conan-official-bzip2-variables.cmake")


set(bzip2_INCLUDE_DIRS_RELEASE "${bzip2_PACKAGE_FOLDER_RELEASE}/include")
set(bzip2_RES_DIRS_RELEASE )
set(bzip2_DEFINITIONS_RELEASE )
set(bzip2_SHARED_LINK_FLAGS_RELEASE )
set(bzip2_EXE_LINK_FLAGS_RELEASE )
set(bzip2_OBJECTS_RELEASE )
set(bzip2_COMPILE_DEFINITIONS_RELEASE )
set(bzip2_COMPILE_OPTIONS_C_RELEASE )
set(bzip2_COMPILE_OPTIONS_CXX_RELEASE )
set(bzip2_LIB_DIRS_RELEASE "${bzip2_PACKAGE_FOLDER_RELEASE}/lib")
set(bzip2_LIBS_RELEASE bz2)
set(bzip2_SYSTEM_LIBS_RELEASE )
set(bzip2_FRAMEWORK_DIRS_RELEASE )
set(bzip2_FRAMEWORKS_RELEASE )
set(bzip2_BUILD_DIRS_RELEASE "${bzip2_PACKAGE_FOLDER_RELEASE}/")

# COMPOUND VARIABLES
set(bzip2_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${bzip2_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${bzip2_COMPILE_OPTIONS_C_RELEASE}>")
set(bzip2_LINKER_FLAGS_RELEASE
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${bzip2_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${bzip2_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${bzip2_EXE_LINK_FLAGS_RELEASE}>")


set(bzip2_COMPONENTS_RELEASE )