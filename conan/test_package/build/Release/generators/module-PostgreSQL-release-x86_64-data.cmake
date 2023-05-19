########### AGGREGATED COMPONENTS AND DEPENDENCIES FOR THE MULTI CONFIG #####################
#############################################################################################

list(APPEND libpq_COMPONENT_NAMES libpq::pgport libpq::pgcommon libpq::pq)
list(REMOVE_DUPLICATES libpq_COMPONENT_NAMES)
set(libpq_FIND_DEPENDENCY_NAMES "")

########### VARIABLES #######################################################################
#############################################################################################
set(libpq_PACKAGE_FOLDER_RELEASE "/Users/alexiprof/.conan/data/libpq/14.5/_/_/package/6841fe8f0f22f6fa260da36a43a94ab525c7ed8d")
set(libpq_BUILD_MODULES_PATHS_RELEASE )


set(libpq_INCLUDE_DIRS_RELEASE "${libpq_PACKAGE_FOLDER_RELEASE}/include")
set(libpq_RES_DIRS_RELEASE )
set(libpq_DEFINITIONS_RELEASE )
set(libpq_SHARED_LINK_FLAGS_RELEASE )
set(libpq_EXE_LINK_FLAGS_RELEASE )
set(libpq_OBJECTS_RELEASE )
set(libpq_COMPILE_DEFINITIONS_RELEASE )
set(libpq_COMPILE_OPTIONS_C_RELEASE )
set(libpq_COMPILE_OPTIONS_CXX_RELEASE )
set(libpq_LIB_DIRS_RELEASE "${libpq_PACKAGE_FOLDER_RELEASE}/lib")
set(libpq_LIBS_RELEASE pq pgcommon pgcommon_shlib pgport pgport_shlib)
set(libpq_SYSTEM_LIBS_RELEASE )
set(libpq_FRAMEWORK_DIRS_RELEASE )
set(libpq_FRAMEWORKS_RELEASE )
set(libpq_BUILD_DIRS_RELEASE )

# COMPOUND VARIABLES
set(libpq_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${libpq_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${libpq_COMPILE_OPTIONS_C_RELEASE}>")
set(libpq_LINKER_FLAGS_RELEASE
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${libpq_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${libpq_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${libpq_EXE_LINK_FLAGS_RELEASE}>")


set(libpq_COMPONENTS_RELEASE libpq::pgport libpq::pgcommon libpq::pq)
########### COMPONENT libpq::pq VARIABLES ############################################

set(libpq_libpq_pq_INCLUDE_DIRS_RELEASE "${libpq_PACKAGE_FOLDER_RELEASE}/include")
set(libpq_libpq_pq_LIB_DIRS_RELEASE "${libpq_PACKAGE_FOLDER_RELEASE}/lib")
set(libpq_libpq_pq_RES_DIRS_RELEASE )
set(libpq_libpq_pq_DEFINITIONS_RELEASE )
set(libpq_libpq_pq_OBJECTS_RELEASE )
set(libpq_libpq_pq_COMPILE_DEFINITIONS_RELEASE )
set(libpq_libpq_pq_COMPILE_OPTIONS_C_RELEASE "")
set(libpq_libpq_pq_COMPILE_OPTIONS_CXX_RELEASE "")
set(libpq_libpq_pq_LIBS_RELEASE pq)
set(libpq_libpq_pq_SYSTEM_LIBS_RELEASE )
set(libpq_libpq_pq_FRAMEWORK_DIRS_RELEASE )
set(libpq_libpq_pq_FRAMEWORKS_RELEASE )
set(libpq_libpq_pq_DEPENDENCIES_RELEASE libpq::pgcommon)
set(libpq_libpq_pq_SHARED_LINK_FLAGS_RELEASE )
set(libpq_libpq_pq_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(libpq_libpq_pq_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${libpq_libpq_pq_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${libpq_libpq_pq_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${libpq_libpq_pq_EXE_LINK_FLAGS_RELEASE}>
)
set(libpq_libpq_pq_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${libpq_libpq_pq_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${libpq_libpq_pq_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT libpq::pgcommon VARIABLES ############################################

set(libpq_libpq_pgcommon_INCLUDE_DIRS_RELEASE "${libpq_PACKAGE_FOLDER_RELEASE}/include")
set(libpq_libpq_pgcommon_LIB_DIRS_RELEASE "${libpq_PACKAGE_FOLDER_RELEASE}/lib")
set(libpq_libpq_pgcommon_RES_DIRS_RELEASE )
set(libpq_libpq_pgcommon_DEFINITIONS_RELEASE )
set(libpq_libpq_pgcommon_OBJECTS_RELEASE )
set(libpq_libpq_pgcommon_COMPILE_DEFINITIONS_RELEASE )
set(libpq_libpq_pgcommon_COMPILE_OPTIONS_C_RELEASE "")
set(libpq_libpq_pgcommon_COMPILE_OPTIONS_CXX_RELEASE "")
set(libpq_libpq_pgcommon_LIBS_RELEASE pgcommon pgcommon_shlib)
set(libpq_libpq_pgcommon_SYSTEM_LIBS_RELEASE )
set(libpq_libpq_pgcommon_FRAMEWORK_DIRS_RELEASE )
set(libpq_libpq_pgcommon_FRAMEWORKS_RELEASE )
set(libpq_libpq_pgcommon_DEPENDENCIES_RELEASE libpq::pgport)
set(libpq_libpq_pgcommon_SHARED_LINK_FLAGS_RELEASE )
set(libpq_libpq_pgcommon_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(libpq_libpq_pgcommon_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${libpq_libpq_pgcommon_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${libpq_libpq_pgcommon_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${libpq_libpq_pgcommon_EXE_LINK_FLAGS_RELEASE}>
)
set(libpq_libpq_pgcommon_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${libpq_libpq_pgcommon_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${libpq_libpq_pgcommon_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT libpq::pgport VARIABLES ############################################

set(libpq_libpq_pgport_INCLUDE_DIRS_RELEASE "${libpq_PACKAGE_FOLDER_RELEASE}/include")
set(libpq_libpq_pgport_LIB_DIRS_RELEASE "${libpq_PACKAGE_FOLDER_RELEASE}/lib")
set(libpq_libpq_pgport_RES_DIRS_RELEASE )
set(libpq_libpq_pgport_DEFINITIONS_RELEASE )
set(libpq_libpq_pgport_OBJECTS_RELEASE )
set(libpq_libpq_pgport_COMPILE_DEFINITIONS_RELEASE )
set(libpq_libpq_pgport_COMPILE_OPTIONS_C_RELEASE "")
set(libpq_libpq_pgport_COMPILE_OPTIONS_CXX_RELEASE "")
set(libpq_libpq_pgport_LIBS_RELEASE pgport pgport_shlib)
set(libpq_libpq_pgport_SYSTEM_LIBS_RELEASE )
set(libpq_libpq_pgport_FRAMEWORK_DIRS_RELEASE )
set(libpq_libpq_pgport_FRAMEWORKS_RELEASE )
set(libpq_libpq_pgport_DEPENDENCIES_RELEASE )
set(libpq_libpq_pgport_SHARED_LINK_FLAGS_RELEASE )
set(libpq_libpq_pgport_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(libpq_libpq_pgport_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${libpq_libpq_pgport_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${libpq_libpq_pgport_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${libpq_libpq_pgport_EXE_LINK_FLAGS_RELEASE}>
)
set(libpq_libpq_pgport_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${libpq_libpq_pgport_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${libpq_libpq_pgport_COMPILE_OPTIONS_C_RELEASE}>")