########### AGGREGATED COMPONENTS AND DEPENDENCIES FOR THE MULTI CONFIG #####################
#############################################################################################

set(jemalloc_COMPONENT_NAMES "")
set(jemalloc_FIND_DEPENDENCY_NAMES "")

########### VARIABLES #######################################################################
#############################################################################################
set(jemalloc_PACKAGE_FOLDER_RELEASE "/Users/alexiprof/.conan/data/jemalloc/5.2.1/_/_/package/63ef65eed81da45031b3c1f57398571a636e3b31")
set(jemalloc_BUILD_MODULES_PATHS_RELEASE )


set(jemalloc_INCLUDE_DIRS_RELEASE "${jemalloc_PACKAGE_FOLDER_RELEASE}/include"
			"${jemalloc_PACKAGE_FOLDER_RELEASE}/include/jemalloc")
set(jemalloc_RES_DIRS_RELEASE )
set(jemalloc_DEFINITIONS_RELEASE "-DJEMALLOC_EXPORT=")
set(jemalloc_SHARED_LINK_FLAGS_RELEASE )
set(jemalloc_EXE_LINK_FLAGS_RELEASE )
set(jemalloc_OBJECTS_RELEASE )
set(jemalloc_COMPILE_DEFINITIONS_RELEASE "JEMALLOC_EXPORT=")
set(jemalloc_COMPILE_OPTIONS_C_RELEASE )
set(jemalloc_COMPILE_OPTIONS_CXX_RELEASE )
set(jemalloc_LIB_DIRS_RELEASE "${jemalloc_PACKAGE_FOLDER_RELEASE}/lib")
set(jemalloc_LIBS_RELEASE jemalloc_pic)
set(jemalloc_SYSTEM_LIBS_RELEASE )
set(jemalloc_FRAMEWORK_DIRS_RELEASE )
set(jemalloc_FRAMEWORKS_RELEASE )
set(jemalloc_BUILD_DIRS_RELEASE "${jemalloc_PACKAGE_FOLDER_RELEASE}/")

# COMPOUND VARIABLES
set(jemalloc_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${jemalloc_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${jemalloc_COMPILE_OPTIONS_C_RELEASE}>")
set(jemalloc_LINKER_FLAGS_RELEASE
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${jemalloc_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${jemalloc_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${jemalloc_EXE_LINK_FLAGS_RELEASE}>")


set(jemalloc_COMPONENTS_RELEASE )