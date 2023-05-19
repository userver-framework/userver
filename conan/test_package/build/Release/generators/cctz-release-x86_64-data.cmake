########### AGGREGATED COMPONENTS AND DEPENDENCIES FOR THE MULTI CONFIG #####################
#############################################################################################

set(cctz_COMPONENT_NAMES "")
set(cctz_FIND_DEPENDENCY_NAMES "")

########### VARIABLES #######################################################################
#############################################################################################
set(cctz_PACKAGE_FOLDER_RELEASE "/Users/alexiprof/.conan/data/cctz/2.3/_/_/package/6f1931e5860f734d55456a9298992b6b0b094a59")
set(cctz_BUILD_MODULES_PATHS_RELEASE )


set(cctz_INCLUDE_DIRS_RELEASE "${cctz_PACKAGE_FOLDER_RELEASE}/include")
set(cctz_RES_DIRS_RELEASE )
set(cctz_DEFINITIONS_RELEASE )
set(cctz_SHARED_LINK_FLAGS_RELEASE )
set(cctz_EXE_LINK_FLAGS_RELEASE )
set(cctz_OBJECTS_RELEASE )
set(cctz_COMPILE_DEFINITIONS_RELEASE )
set(cctz_COMPILE_OPTIONS_C_RELEASE )
set(cctz_COMPILE_OPTIONS_CXX_RELEASE )
set(cctz_LIB_DIRS_RELEASE "${cctz_PACKAGE_FOLDER_RELEASE}/lib")
set(cctz_LIBS_RELEASE cctz)
set(cctz_SYSTEM_LIBS_RELEASE )
set(cctz_FRAMEWORK_DIRS_RELEASE )
set(cctz_FRAMEWORKS_RELEASE CoreFoundation)
set(cctz_BUILD_DIRS_RELEASE "${cctz_PACKAGE_FOLDER_RELEASE}/")

# COMPOUND VARIABLES
set(cctz_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${cctz_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${cctz_COMPILE_OPTIONS_C_RELEASE}>")
set(cctz_LINKER_FLAGS_RELEASE
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${cctz_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${cctz_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${cctz_EXE_LINK_FLAGS_RELEASE}>")


set(cctz_COMPONENTS_RELEASE )