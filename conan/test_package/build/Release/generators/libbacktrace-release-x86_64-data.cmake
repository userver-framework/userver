########### AGGREGATED COMPONENTS AND DEPENDENCIES FOR THE MULTI CONFIG #####################
#############################################################################################

set(libbacktrace_COMPONENT_NAMES "")
set(libbacktrace_FIND_DEPENDENCY_NAMES "")

########### VARIABLES #######################################################################
#############################################################################################
set(libbacktrace_PACKAGE_FOLDER_RELEASE "/Users/alexiprof/.conan/data/libbacktrace/cci.20210118/_/_/package/6841fe8f0f22f6fa260da36a43a94ab525c7ed8d")
set(libbacktrace_BUILD_MODULES_PATHS_RELEASE )


set(libbacktrace_INCLUDE_DIRS_RELEASE "${libbacktrace_PACKAGE_FOLDER_RELEASE}/include")
set(libbacktrace_RES_DIRS_RELEASE )
set(libbacktrace_DEFINITIONS_RELEASE )
set(libbacktrace_SHARED_LINK_FLAGS_RELEASE )
set(libbacktrace_EXE_LINK_FLAGS_RELEASE )
set(libbacktrace_OBJECTS_RELEASE )
set(libbacktrace_COMPILE_DEFINITIONS_RELEASE )
set(libbacktrace_COMPILE_OPTIONS_C_RELEASE )
set(libbacktrace_COMPILE_OPTIONS_CXX_RELEASE )
set(libbacktrace_LIB_DIRS_RELEASE "${libbacktrace_PACKAGE_FOLDER_RELEASE}/lib")
set(libbacktrace_LIBS_RELEASE backtrace)
set(libbacktrace_SYSTEM_LIBS_RELEASE )
set(libbacktrace_FRAMEWORK_DIRS_RELEASE )
set(libbacktrace_FRAMEWORKS_RELEASE )
set(libbacktrace_BUILD_DIRS_RELEASE "${libbacktrace_PACKAGE_FOLDER_RELEASE}/")

# COMPOUND VARIABLES
set(libbacktrace_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${libbacktrace_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${libbacktrace_COMPILE_OPTIONS_C_RELEASE}>")
set(libbacktrace_LINKER_FLAGS_RELEASE
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${libbacktrace_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${libbacktrace_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${libbacktrace_EXE_LINK_FLAGS_RELEASE}>")


set(libbacktrace_COMPONENTS_RELEASE )