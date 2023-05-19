########### AGGREGATED COMPONENTS AND DEPENDENCIES FOR THE MULTI CONFIG #####################
#############################################################################################

list(APPEND snappy_COMPONENT_NAMES Snappy::snappy)
list(REMOVE_DUPLICATES snappy_COMPONENT_NAMES)
set(snappy_FIND_DEPENDENCY_NAMES "")

########### VARIABLES #######################################################################
#############################################################################################
set(snappy_PACKAGE_FOLDER_RELEASE "/Users/alexiprof/.conan/data/snappy/1.1.10/_/_/package/3782cb8c3754bcc4f0ca2283b503d86479835458")
set(snappy_BUILD_MODULES_PATHS_RELEASE )


set(snappy_INCLUDE_DIRS_RELEASE "${snappy_PACKAGE_FOLDER_RELEASE}/include")
set(snappy_RES_DIRS_RELEASE )
set(snappy_DEFINITIONS_RELEASE )
set(snappy_SHARED_LINK_FLAGS_RELEASE )
set(snappy_EXE_LINK_FLAGS_RELEASE )
set(snappy_OBJECTS_RELEASE )
set(snappy_COMPILE_DEFINITIONS_RELEASE )
set(snappy_COMPILE_OPTIONS_C_RELEASE )
set(snappy_COMPILE_OPTIONS_CXX_RELEASE )
set(snappy_LIB_DIRS_RELEASE "${snappy_PACKAGE_FOLDER_RELEASE}/lib")
set(snappy_LIBS_RELEASE snappy)
set(snappy_SYSTEM_LIBS_RELEASE c++)
set(snappy_FRAMEWORK_DIRS_RELEASE )
set(snappy_FRAMEWORKS_RELEASE )
set(snappy_BUILD_DIRS_RELEASE )

# COMPOUND VARIABLES
set(snappy_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${snappy_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${snappy_COMPILE_OPTIONS_C_RELEASE}>")
set(snappy_LINKER_FLAGS_RELEASE
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${snappy_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${snappy_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${snappy_EXE_LINK_FLAGS_RELEASE}>")


set(snappy_COMPONENTS_RELEASE Snappy::snappy)
########### COMPONENT Snappy::snappy VARIABLES ############################################

set(snappy_Snappy_snappy_INCLUDE_DIRS_RELEASE "${snappy_PACKAGE_FOLDER_RELEASE}/include")
set(snappy_Snappy_snappy_LIB_DIRS_RELEASE "${snappy_PACKAGE_FOLDER_RELEASE}/lib")
set(snappy_Snappy_snappy_RES_DIRS_RELEASE )
set(snappy_Snappy_snappy_DEFINITIONS_RELEASE )
set(snappy_Snappy_snappy_OBJECTS_RELEASE )
set(snappy_Snappy_snappy_COMPILE_DEFINITIONS_RELEASE )
set(snappy_Snappy_snappy_COMPILE_OPTIONS_C_RELEASE "")
set(snappy_Snappy_snappy_COMPILE_OPTIONS_CXX_RELEASE "")
set(snappy_Snappy_snappy_LIBS_RELEASE snappy)
set(snappy_Snappy_snappy_SYSTEM_LIBS_RELEASE c++)
set(snappy_Snappy_snappy_FRAMEWORK_DIRS_RELEASE )
set(snappy_Snappy_snappy_FRAMEWORKS_RELEASE )
set(snappy_Snappy_snappy_DEPENDENCIES_RELEASE )
set(snappy_Snappy_snappy_SHARED_LINK_FLAGS_RELEASE )
set(snappy_Snappy_snappy_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(snappy_Snappy_snappy_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${snappy_Snappy_snappy_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${snappy_Snappy_snappy_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${snappy_Snappy_snappy_EXE_LINK_FLAGS_RELEASE}>
)
set(snappy_Snappy_snappy_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${snappy_Snappy_snappy_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${snappy_Snappy_snappy_COMPILE_OPTIONS_C_RELEASE}>")