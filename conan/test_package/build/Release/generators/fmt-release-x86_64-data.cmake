########### AGGREGATED COMPONENTS AND DEPENDENCIES FOR THE MULTI CONFIG #####################
#############################################################################################

list(APPEND fmt_COMPONENT_NAMES fmt::fmt)
list(REMOVE_DUPLICATES fmt_COMPONENT_NAMES)
set(fmt_FIND_DEPENDENCY_NAMES "")

########### VARIABLES #######################################################################
#############################################################################################
set(fmt_PACKAGE_FOLDER_RELEASE "/Users/alexiprof/.conan/data/fmt/8.1.1/_/_/package/80138d4a58def120da0b8c9199f2b7a4e464a85b")
set(fmt_BUILD_MODULES_PATHS_RELEASE )


set(fmt_INCLUDE_DIRS_RELEASE "${fmt_PACKAGE_FOLDER_RELEASE}/include")
set(fmt_RES_DIRS_RELEASE )
set(fmt_DEFINITIONS_RELEASE )
set(fmt_SHARED_LINK_FLAGS_RELEASE )
set(fmt_EXE_LINK_FLAGS_RELEASE )
set(fmt_OBJECTS_RELEASE )
set(fmt_COMPILE_DEFINITIONS_RELEASE )
set(fmt_COMPILE_OPTIONS_C_RELEASE )
set(fmt_COMPILE_OPTIONS_CXX_RELEASE )
set(fmt_LIB_DIRS_RELEASE "${fmt_PACKAGE_FOLDER_RELEASE}/lib")
set(fmt_LIBS_RELEASE fmt)
set(fmt_SYSTEM_LIBS_RELEASE )
set(fmt_FRAMEWORK_DIRS_RELEASE )
set(fmt_FRAMEWORKS_RELEASE )
set(fmt_BUILD_DIRS_RELEASE )

# COMPOUND VARIABLES
set(fmt_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${fmt_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${fmt_COMPILE_OPTIONS_C_RELEASE}>")
set(fmt_LINKER_FLAGS_RELEASE
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${fmt_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${fmt_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${fmt_EXE_LINK_FLAGS_RELEASE}>")


set(fmt_COMPONENTS_RELEASE fmt::fmt)
########### COMPONENT fmt::fmt VARIABLES ############################################

set(fmt_fmt_fmt_INCLUDE_DIRS_RELEASE "${fmt_PACKAGE_FOLDER_RELEASE}/include")
set(fmt_fmt_fmt_LIB_DIRS_RELEASE "${fmt_PACKAGE_FOLDER_RELEASE}/lib")
set(fmt_fmt_fmt_RES_DIRS_RELEASE )
set(fmt_fmt_fmt_DEFINITIONS_RELEASE )
set(fmt_fmt_fmt_OBJECTS_RELEASE )
set(fmt_fmt_fmt_COMPILE_DEFINITIONS_RELEASE )
set(fmt_fmt_fmt_COMPILE_OPTIONS_C_RELEASE "")
set(fmt_fmt_fmt_COMPILE_OPTIONS_CXX_RELEASE "")
set(fmt_fmt_fmt_LIBS_RELEASE fmt)
set(fmt_fmt_fmt_SYSTEM_LIBS_RELEASE )
set(fmt_fmt_fmt_FRAMEWORK_DIRS_RELEASE )
set(fmt_fmt_fmt_FRAMEWORKS_RELEASE )
set(fmt_fmt_fmt_DEPENDENCIES_RELEASE )
set(fmt_fmt_fmt_SHARED_LINK_FLAGS_RELEASE )
set(fmt_fmt_fmt_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(fmt_fmt_fmt_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${fmt_fmt_fmt_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${fmt_fmt_fmt_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${fmt_fmt_fmt_EXE_LINK_FLAGS_RELEASE}>
)
set(fmt_fmt_fmt_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${fmt_fmt_fmt_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${fmt_fmt_fmt_COMPILE_OPTIONS_C_RELEASE}>")