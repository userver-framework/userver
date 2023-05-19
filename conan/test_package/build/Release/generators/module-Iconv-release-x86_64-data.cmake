########### AGGREGATED COMPONENTS AND DEPENDENCIES FOR THE MULTI CONFIG #####################
#############################################################################################

set(libiconv_COMPONENT_NAMES "")
set(libiconv_FIND_DEPENDENCY_NAMES "")

########### VARIABLES #######################################################################
#############################################################################################
set(libiconv_PACKAGE_FOLDER_RELEASE "/Users/alexiprof/.conan/data/libiconv/1.17/_/_/package/6841fe8f0f22f6fa260da36a43a94ab525c7ed8d")
set(libiconv_BUILD_MODULES_PATHS_RELEASE )


set(libiconv_INCLUDE_DIRS_RELEASE "${libiconv_PACKAGE_FOLDER_RELEASE}/include")
set(libiconv_RES_DIRS_RELEASE )
set(libiconv_DEFINITIONS_RELEASE )
set(libiconv_SHARED_LINK_FLAGS_RELEASE )
set(libiconv_EXE_LINK_FLAGS_RELEASE )
set(libiconv_OBJECTS_RELEASE )
set(libiconv_COMPILE_DEFINITIONS_RELEASE )
set(libiconv_COMPILE_OPTIONS_C_RELEASE )
set(libiconv_COMPILE_OPTIONS_CXX_RELEASE )
set(libiconv_LIB_DIRS_RELEASE "${libiconv_PACKAGE_FOLDER_RELEASE}/lib")
set(libiconv_LIBS_RELEASE iconv charset)
set(libiconv_SYSTEM_LIBS_RELEASE )
set(libiconv_FRAMEWORK_DIRS_RELEASE )
set(libiconv_FRAMEWORKS_RELEASE )
set(libiconv_BUILD_DIRS_RELEASE "${libiconv_PACKAGE_FOLDER_RELEASE}/")

# COMPOUND VARIABLES
set(libiconv_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${libiconv_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${libiconv_COMPILE_OPTIONS_C_RELEASE}>")
set(libiconv_LINKER_FLAGS_RELEASE
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${libiconv_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${libiconv_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${libiconv_EXE_LINK_FLAGS_RELEASE}>")


set(libiconv_COMPONENTS_RELEASE )