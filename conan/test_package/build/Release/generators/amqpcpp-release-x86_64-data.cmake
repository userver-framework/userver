########### AGGREGATED COMPONENTS AND DEPENDENCIES FOR THE MULTI CONFIG #####################
#############################################################################################

set(amqp-cpp_COMPONENT_NAMES "")
list(APPEND amqp-cpp_FIND_DEPENDENCY_NAMES OpenSSL)
list(REMOVE_DUPLICATES amqp-cpp_FIND_DEPENDENCY_NAMES)
set(OpenSSL_FIND_MODE "NO_MODULE")

########### VARIABLES #######################################################################
#############################################################################################
set(amqp-cpp_PACKAGE_FOLDER_RELEASE "/Users/alexiprof/.conan/data/amqp-cpp/4.3.16/_/_/package/9955bffe79339c531342a3a20c0a10a1e5a3a2e0")
set(amqp-cpp_BUILD_MODULES_PATHS_RELEASE )


set(amqp-cpp_INCLUDE_DIRS_RELEASE "${amqp-cpp_PACKAGE_FOLDER_RELEASE}/include")
set(amqp-cpp_RES_DIRS_RELEASE )
set(amqp-cpp_DEFINITIONS_RELEASE )
set(amqp-cpp_SHARED_LINK_FLAGS_RELEASE )
set(amqp-cpp_EXE_LINK_FLAGS_RELEASE )
set(amqp-cpp_OBJECTS_RELEASE )
set(amqp-cpp_COMPILE_DEFINITIONS_RELEASE )
set(amqp-cpp_COMPILE_OPTIONS_C_RELEASE )
set(amqp-cpp_COMPILE_OPTIONS_CXX_RELEASE )
set(amqp-cpp_LIB_DIRS_RELEASE "${amqp-cpp_PACKAGE_FOLDER_RELEASE}/lib")
set(amqp-cpp_LIBS_RELEASE amqpcpp)
set(amqp-cpp_SYSTEM_LIBS_RELEASE )
set(amqp-cpp_FRAMEWORK_DIRS_RELEASE )
set(amqp-cpp_FRAMEWORKS_RELEASE )
set(amqp-cpp_BUILD_DIRS_RELEASE "${amqp-cpp_PACKAGE_FOLDER_RELEASE}/")

# COMPOUND VARIABLES
set(amqp-cpp_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${amqp-cpp_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${amqp-cpp_COMPILE_OPTIONS_C_RELEASE}>")
set(amqp-cpp_LINKER_FLAGS_RELEASE
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${amqp-cpp_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${amqp-cpp_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${amqp-cpp_EXE_LINK_FLAGS_RELEASE}>")


set(amqp-cpp_COMPONENTS_RELEASE )