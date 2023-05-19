########### AGGREGATED COMPONENTS AND DEPENDENCIES FOR THE MULTI CONFIG #####################
#############################################################################################

set(cyrus-sasl_COMPONENT_NAMES "")
list(APPEND cyrus-sasl_FIND_DEPENDENCY_NAMES OpenSSL)
list(REMOVE_DUPLICATES cyrus-sasl_FIND_DEPENDENCY_NAMES)
set(OpenSSL_FIND_MODE "NO_MODULE")

########### VARIABLES #######################################################################
#############################################################################################
set(cyrus-sasl_PACKAGE_FOLDER_RELEASE "/Users/alexiprof/.conan/data/cyrus-sasl/2.1.27/_/_/package/7e7af0ed7931a161b325bff1e2f6c62e8ff7cc0a")
set(cyrus-sasl_BUILD_MODULES_PATHS_RELEASE )


set(cyrus-sasl_INCLUDE_DIRS_RELEASE "${cyrus-sasl_PACKAGE_FOLDER_RELEASE}/include")
set(cyrus-sasl_RES_DIRS_RELEASE )
set(cyrus-sasl_DEFINITIONS_RELEASE )
set(cyrus-sasl_SHARED_LINK_FLAGS_RELEASE )
set(cyrus-sasl_EXE_LINK_FLAGS_RELEASE )
set(cyrus-sasl_OBJECTS_RELEASE )
set(cyrus-sasl_COMPILE_DEFINITIONS_RELEASE )
set(cyrus-sasl_COMPILE_OPTIONS_C_RELEASE )
set(cyrus-sasl_COMPILE_OPTIONS_CXX_RELEASE )
set(cyrus-sasl_LIB_DIRS_RELEASE "${cyrus-sasl_PACKAGE_FOLDER_RELEASE}/lib")
set(cyrus-sasl_LIBS_RELEASE sasl2)
set(cyrus-sasl_SYSTEM_LIBS_RELEASE )
set(cyrus-sasl_FRAMEWORK_DIRS_RELEASE )
set(cyrus-sasl_FRAMEWORKS_RELEASE )
set(cyrus-sasl_BUILD_DIRS_RELEASE "${cyrus-sasl_PACKAGE_FOLDER_RELEASE}/")

# COMPOUND VARIABLES
set(cyrus-sasl_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${cyrus-sasl_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${cyrus-sasl_COMPILE_OPTIONS_C_RELEASE}>")
set(cyrus-sasl_LINKER_FLAGS_RELEASE
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${cyrus-sasl_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${cyrus-sasl_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${cyrus-sasl_EXE_LINK_FLAGS_RELEASE}>")


set(cyrus-sasl_COMPONENTS_RELEASE )