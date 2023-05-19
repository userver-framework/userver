########### AGGREGATED COMPONENTS AND DEPENDENCIES FOR THE MULTI CONFIG #####################
#############################################################################################

list(APPEND hiredis_COMPONENT_NAMES hiredis::hiredis)
list(REMOVE_DUPLICATES hiredis_COMPONENT_NAMES)
set(hiredis_FIND_DEPENDENCY_NAMES "")

########### VARIABLES #######################################################################
#############################################################################################
set(hiredis_PACKAGE_FOLDER_RELEASE "/Users/alexiprof/.conan/data/hiredis/1.0.2/_/_/package/6841fe8f0f22f6fa260da36a43a94ab525c7ed8d")
set(hiredis_BUILD_MODULES_PATHS_RELEASE )


set(hiredis_INCLUDE_DIRS_RELEASE "${hiredis_PACKAGE_FOLDER_RELEASE}/include")
set(hiredis_RES_DIRS_RELEASE )
set(hiredis_DEFINITIONS_RELEASE )
set(hiredis_SHARED_LINK_FLAGS_RELEASE )
set(hiredis_EXE_LINK_FLAGS_RELEASE )
set(hiredis_OBJECTS_RELEASE )
set(hiredis_COMPILE_DEFINITIONS_RELEASE )
set(hiredis_COMPILE_OPTIONS_C_RELEASE )
set(hiredis_COMPILE_OPTIONS_CXX_RELEASE )
set(hiredis_LIB_DIRS_RELEASE "${hiredis_PACKAGE_FOLDER_RELEASE}/lib")
set(hiredis_LIBS_RELEASE hiredis)
set(hiredis_SYSTEM_LIBS_RELEASE )
set(hiredis_FRAMEWORK_DIRS_RELEASE )
set(hiredis_FRAMEWORKS_RELEASE )
set(hiredis_BUILD_DIRS_RELEASE )

# COMPOUND VARIABLES
set(hiredis_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${hiredis_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${hiredis_COMPILE_OPTIONS_C_RELEASE}>")
set(hiredis_LINKER_FLAGS_RELEASE
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${hiredis_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${hiredis_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${hiredis_EXE_LINK_FLAGS_RELEASE}>")


set(hiredis_COMPONENTS_RELEASE hiredis::hiredis)
########### COMPONENT hiredis::hiredis VARIABLES ############################################

set(hiredis_hiredis_hiredis_INCLUDE_DIRS_RELEASE "${hiredis_PACKAGE_FOLDER_RELEASE}/include")
set(hiredis_hiredis_hiredis_LIB_DIRS_RELEASE "${hiredis_PACKAGE_FOLDER_RELEASE}/lib")
set(hiredis_hiredis_hiredis_RES_DIRS_RELEASE )
set(hiredis_hiredis_hiredis_DEFINITIONS_RELEASE )
set(hiredis_hiredis_hiredis_OBJECTS_RELEASE )
set(hiredis_hiredis_hiredis_COMPILE_DEFINITIONS_RELEASE )
set(hiredis_hiredis_hiredis_COMPILE_OPTIONS_C_RELEASE "")
set(hiredis_hiredis_hiredis_COMPILE_OPTIONS_CXX_RELEASE "")
set(hiredis_hiredis_hiredis_LIBS_RELEASE hiredis)
set(hiredis_hiredis_hiredis_SYSTEM_LIBS_RELEASE )
set(hiredis_hiredis_hiredis_FRAMEWORK_DIRS_RELEASE )
set(hiredis_hiredis_hiredis_FRAMEWORKS_RELEASE )
set(hiredis_hiredis_hiredis_DEPENDENCIES_RELEASE )
set(hiredis_hiredis_hiredis_SHARED_LINK_FLAGS_RELEASE )
set(hiredis_hiredis_hiredis_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(hiredis_hiredis_hiredis_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${hiredis_hiredis_hiredis_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${hiredis_hiredis_hiredis_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${hiredis_hiredis_hiredis_EXE_LINK_FLAGS_RELEASE}>
)
set(hiredis_hiredis_hiredis_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${hiredis_hiredis_hiredis_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${hiredis_hiredis_hiredis_COMPILE_OPTIONS_C_RELEASE}>")