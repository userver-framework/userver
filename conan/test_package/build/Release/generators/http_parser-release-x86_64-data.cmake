########### AGGREGATED COMPONENTS AND DEPENDENCIES FOR THE MULTI CONFIG #####################
#############################################################################################

set(http_parser_COMPONENT_NAMES "")
set(http_parser_FIND_DEPENDENCY_NAMES "")

########### VARIABLES #######################################################################
#############################################################################################
set(http_parser_PACKAGE_FOLDER_RELEASE "/Users/alexiprof/.conan/data/http_parser/2.9.4/_/_/package/6841fe8f0f22f6fa260da36a43a94ab525c7ed8d")
set(http_parser_BUILD_MODULES_PATHS_RELEASE )


set(http_parser_INCLUDE_DIRS_RELEASE "${http_parser_PACKAGE_FOLDER_RELEASE}/include")
set(http_parser_RES_DIRS_RELEASE )
set(http_parser_DEFINITIONS_RELEASE )
set(http_parser_SHARED_LINK_FLAGS_RELEASE )
set(http_parser_EXE_LINK_FLAGS_RELEASE )
set(http_parser_OBJECTS_RELEASE )
set(http_parser_COMPILE_DEFINITIONS_RELEASE )
set(http_parser_COMPILE_OPTIONS_C_RELEASE )
set(http_parser_COMPILE_OPTIONS_CXX_RELEASE )
set(http_parser_LIB_DIRS_RELEASE "${http_parser_PACKAGE_FOLDER_RELEASE}/lib")
set(http_parser_LIBS_RELEASE http_parser)
set(http_parser_SYSTEM_LIBS_RELEASE )
set(http_parser_FRAMEWORK_DIRS_RELEASE )
set(http_parser_FRAMEWORKS_RELEASE )
set(http_parser_BUILD_DIRS_RELEASE "${http_parser_PACKAGE_FOLDER_RELEASE}/")

# COMPOUND VARIABLES
set(http_parser_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${http_parser_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${http_parser_COMPILE_OPTIONS_C_RELEASE}>")
set(http_parser_LINKER_FLAGS_RELEASE
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${http_parser_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${http_parser_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${http_parser_EXE_LINK_FLAGS_RELEASE}>")


set(http_parser_COMPONENTS_RELEASE )