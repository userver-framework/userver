########### AGGREGATED COMPONENTS AND DEPENDENCIES FOR THE MULTI CONFIG #####################
#############################################################################################

list(APPEND protobuf_COMPONENT_NAMES protobuf::libprotobuf protobuf::libprotoc)
list(REMOVE_DUPLICATES protobuf_COMPONENT_NAMES)
list(APPEND protobuf_FIND_DEPENDENCY_NAMES ZLIB)
list(REMOVE_DUPLICATES protobuf_FIND_DEPENDENCY_NAMES)
set(ZLIB_FIND_MODE "NO_MODULE")

########### VARIABLES #######################################################################
#############################################################################################
set(protobuf_PACKAGE_FOLDER_RELEASE "/Users/alexiprof/.conan/data/protobuf/3.21.4/_/_/package/8264a6a82f1a58ddbf1d543f69f549b8e37e8e6a")
set(protobuf_BUILD_MODULES_PATHS_RELEASE "${protobuf_PACKAGE_FOLDER_RELEASE}/lib/cmake/protobuf/protobuf-generate.cmake"
			"${protobuf_PACKAGE_FOLDER_RELEASE}/lib/cmake/protobuf/protobuf-module.cmake"
			"${protobuf_PACKAGE_FOLDER_RELEASE}/lib/cmake/protobuf/protobuf-options.cmake")


set(protobuf_INCLUDE_DIRS_RELEASE "${protobuf_PACKAGE_FOLDER_RELEASE}/include")
set(protobuf_RES_DIRS_RELEASE )
set(protobuf_DEFINITIONS_RELEASE )
set(protobuf_SHARED_LINK_FLAGS_RELEASE )
set(protobuf_EXE_LINK_FLAGS_RELEASE )
set(protobuf_OBJECTS_RELEASE )
set(protobuf_COMPILE_DEFINITIONS_RELEASE )
set(protobuf_COMPILE_OPTIONS_C_RELEASE )
set(protobuf_COMPILE_OPTIONS_CXX_RELEASE )
set(protobuf_LIB_DIRS_RELEASE "${protobuf_PACKAGE_FOLDER_RELEASE}/lib")
set(protobuf_LIBS_RELEASE protoc protobuf)
set(protobuf_SYSTEM_LIBS_RELEASE )
set(protobuf_FRAMEWORK_DIRS_RELEASE )
set(protobuf_FRAMEWORKS_RELEASE )
set(protobuf_BUILD_DIRS_RELEASE "${protobuf_PACKAGE_FOLDER_RELEASE}/lib/cmake/protobuf")

# COMPOUND VARIABLES
set(protobuf_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${protobuf_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${protobuf_COMPILE_OPTIONS_C_RELEASE}>")
set(protobuf_LINKER_FLAGS_RELEASE
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${protobuf_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${protobuf_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${protobuf_EXE_LINK_FLAGS_RELEASE}>")


set(protobuf_COMPONENTS_RELEASE protobuf::libprotobuf protobuf::libprotoc)
########### COMPONENT protobuf::libprotoc VARIABLES ############################################

set(protobuf_protobuf_libprotoc_INCLUDE_DIRS_RELEASE "${protobuf_PACKAGE_FOLDER_RELEASE}/include")
set(protobuf_protobuf_libprotoc_LIB_DIRS_RELEASE "${protobuf_PACKAGE_FOLDER_RELEASE}/lib")
set(protobuf_protobuf_libprotoc_RES_DIRS_RELEASE )
set(protobuf_protobuf_libprotoc_DEFINITIONS_RELEASE )
set(protobuf_protobuf_libprotoc_OBJECTS_RELEASE )
set(protobuf_protobuf_libprotoc_COMPILE_DEFINITIONS_RELEASE )
set(protobuf_protobuf_libprotoc_COMPILE_OPTIONS_C_RELEASE "")
set(protobuf_protobuf_libprotoc_COMPILE_OPTIONS_CXX_RELEASE "")
set(protobuf_protobuf_libprotoc_LIBS_RELEASE protoc)
set(protobuf_protobuf_libprotoc_SYSTEM_LIBS_RELEASE )
set(protobuf_protobuf_libprotoc_FRAMEWORK_DIRS_RELEASE )
set(protobuf_protobuf_libprotoc_FRAMEWORKS_RELEASE )
set(protobuf_protobuf_libprotoc_DEPENDENCIES_RELEASE protobuf::libprotobuf)
set(protobuf_protobuf_libprotoc_SHARED_LINK_FLAGS_RELEASE )
set(protobuf_protobuf_libprotoc_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(protobuf_protobuf_libprotoc_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${protobuf_protobuf_libprotoc_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${protobuf_protobuf_libprotoc_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${protobuf_protobuf_libprotoc_EXE_LINK_FLAGS_RELEASE}>
)
set(protobuf_protobuf_libprotoc_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${protobuf_protobuf_libprotoc_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${protobuf_protobuf_libprotoc_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT protobuf::libprotobuf VARIABLES ############################################

set(protobuf_protobuf_libprotobuf_INCLUDE_DIRS_RELEASE "${protobuf_PACKAGE_FOLDER_RELEASE}/include")
set(protobuf_protobuf_libprotobuf_LIB_DIRS_RELEASE "${protobuf_PACKAGE_FOLDER_RELEASE}/lib")
set(protobuf_protobuf_libprotobuf_RES_DIRS_RELEASE )
set(protobuf_protobuf_libprotobuf_DEFINITIONS_RELEASE )
set(protobuf_protobuf_libprotobuf_OBJECTS_RELEASE )
set(protobuf_protobuf_libprotobuf_COMPILE_DEFINITIONS_RELEASE )
set(protobuf_protobuf_libprotobuf_COMPILE_OPTIONS_C_RELEASE "")
set(protobuf_protobuf_libprotobuf_COMPILE_OPTIONS_CXX_RELEASE "")
set(protobuf_protobuf_libprotobuf_LIBS_RELEASE protobuf)
set(protobuf_protobuf_libprotobuf_SYSTEM_LIBS_RELEASE )
set(protobuf_protobuf_libprotobuf_FRAMEWORK_DIRS_RELEASE )
set(protobuf_protobuf_libprotobuf_FRAMEWORKS_RELEASE )
set(protobuf_protobuf_libprotobuf_DEPENDENCIES_RELEASE ZLIB::ZLIB)
set(protobuf_protobuf_libprotobuf_SHARED_LINK_FLAGS_RELEASE )
set(protobuf_protobuf_libprotobuf_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(protobuf_protobuf_libprotobuf_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${protobuf_protobuf_libprotobuf_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${protobuf_protobuf_libprotobuf_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${protobuf_protobuf_libprotobuf_EXE_LINK_FLAGS_RELEASE}>
)
set(protobuf_protobuf_libprotobuf_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${protobuf_protobuf_libprotobuf_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${protobuf_protobuf_libprotobuf_COMPILE_OPTIONS_C_RELEASE}>")