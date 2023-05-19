########### AGGREGATED COMPONENTS AND DEPENDENCIES FOR THE MULTI CONFIG #####################
#############################################################################################

set(yaml-cpp_COMPONENT_NAMES "")
set(yaml-cpp_FIND_DEPENDENCY_NAMES "")

########### VARIABLES #######################################################################
#############################################################################################
set(yaml-cpp_PACKAGE_FOLDER_RELEASE "/Users/alexiprof/.conan/data/yaml-cpp/0.7.0/_/_/package/3782cb8c3754bcc4f0ca2283b503d86479835458")
set(yaml-cpp_BUILD_MODULES_PATHS_RELEASE )


set(yaml-cpp_INCLUDE_DIRS_RELEASE "${yaml-cpp_PACKAGE_FOLDER_RELEASE}/include")
set(yaml-cpp_RES_DIRS_RELEASE )
set(yaml-cpp_DEFINITIONS_RELEASE )
set(yaml-cpp_SHARED_LINK_FLAGS_RELEASE )
set(yaml-cpp_EXE_LINK_FLAGS_RELEASE )
set(yaml-cpp_OBJECTS_RELEASE )
set(yaml-cpp_COMPILE_DEFINITIONS_RELEASE )
set(yaml-cpp_COMPILE_OPTIONS_C_RELEASE )
set(yaml-cpp_COMPILE_OPTIONS_CXX_RELEASE )
set(yaml-cpp_LIB_DIRS_RELEASE "${yaml-cpp_PACKAGE_FOLDER_RELEASE}/lib")
set(yaml-cpp_LIBS_RELEASE yaml-cpp)
set(yaml-cpp_SYSTEM_LIBS_RELEASE )
set(yaml-cpp_FRAMEWORK_DIRS_RELEASE )
set(yaml-cpp_FRAMEWORKS_RELEASE )
set(yaml-cpp_BUILD_DIRS_RELEASE "${yaml-cpp_PACKAGE_FOLDER_RELEASE}/")

# COMPOUND VARIABLES
set(yaml-cpp_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${yaml-cpp_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${yaml-cpp_COMPILE_OPTIONS_C_RELEASE}>")
set(yaml-cpp_LINKER_FLAGS_RELEASE
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${yaml-cpp_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${yaml-cpp_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${yaml-cpp_EXE_LINK_FLAGS_RELEASE}>")


set(yaml-cpp_COMPONENTS_RELEASE )