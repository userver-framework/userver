########### AGGREGATED COMPONENTS AND DEPENDENCIES FOR THE MULTI CONFIG #####################
#############################################################################################

list(APPEND benchmark_COMPONENT_NAMES benchmark::benchmark benchmark::benchmark_main)
list(REMOVE_DUPLICATES benchmark_COMPONENT_NAMES)
set(benchmark_FIND_DEPENDENCY_NAMES "")

########### VARIABLES #######################################################################
#############################################################################################
set(benchmark_PACKAGE_FOLDER_RELEASE "/Users/alexiprof/.conan/data/benchmark/1.6.2/_/_/package/4939359d58984e7dd11a2299dc27409aada19471")
set(benchmark_BUILD_MODULES_PATHS_RELEASE )


set(benchmark_INCLUDE_DIRS_RELEASE "${benchmark_PACKAGE_FOLDER_RELEASE}/include")
set(benchmark_RES_DIRS_RELEASE )
set(benchmark_DEFINITIONS_RELEASE )
set(benchmark_SHARED_LINK_FLAGS_RELEASE )
set(benchmark_EXE_LINK_FLAGS_RELEASE )
set(benchmark_OBJECTS_RELEASE )
set(benchmark_COMPILE_DEFINITIONS_RELEASE )
set(benchmark_COMPILE_OPTIONS_C_RELEASE )
set(benchmark_COMPILE_OPTIONS_CXX_RELEASE )
set(benchmark_LIB_DIRS_RELEASE "${benchmark_PACKAGE_FOLDER_RELEASE}/lib")
set(benchmark_LIBS_RELEASE benchmark_main benchmark)
set(benchmark_SYSTEM_LIBS_RELEASE )
set(benchmark_FRAMEWORK_DIRS_RELEASE )
set(benchmark_FRAMEWORKS_RELEASE )
set(benchmark_BUILD_DIRS_RELEASE )

# COMPOUND VARIABLES
set(benchmark_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${benchmark_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${benchmark_COMPILE_OPTIONS_C_RELEASE}>")
set(benchmark_LINKER_FLAGS_RELEASE
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${benchmark_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${benchmark_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${benchmark_EXE_LINK_FLAGS_RELEASE}>")


set(benchmark_COMPONENTS_RELEASE benchmark::benchmark benchmark::benchmark_main)
########### COMPONENT benchmark::benchmark_main VARIABLES ############################################

set(benchmark_benchmark_benchmark_main_INCLUDE_DIRS_RELEASE "${benchmark_PACKAGE_FOLDER_RELEASE}/include")
set(benchmark_benchmark_benchmark_main_LIB_DIRS_RELEASE "${benchmark_PACKAGE_FOLDER_RELEASE}/lib")
set(benchmark_benchmark_benchmark_main_RES_DIRS_RELEASE )
set(benchmark_benchmark_benchmark_main_DEFINITIONS_RELEASE )
set(benchmark_benchmark_benchmark_main_OBJECTS_RELEASE )
set(benchmark_benchmark_benchmark_main_COMPILE_DEFINITIONS_RELEASE )
set(benchmark_benchmark_benchmark_main_COMPILE_OPTIONS_C_RELEASE "")
set(benchmark_benchmark_benchmark_main_COMPILE_OPTIONS_CXX_RELEASE "")
set(benchmark_benchmark_benchmark_main_LIBS_RELEASE benchmark_main)
set(benchmark_benchmark_benchmark_main_SYSTEM_LIBS_RELEASE )
set(benchmark_benchmark_benchmark_main_FRAMEWORK_DIRS_RELEASE )
set(benchmark_benchmark_benchmark_main_FRAMEWORKS_RELEASE )
set(benchmark_benchmark_benchmark_main_DEPENDENCIES_RELEASE benchmark::benchmark)
set(benchmark_benchmark_benchmark_main_SHARED_LINK_FLAGS_RELEASE )
set(benchmark_benchmark_benchmark_main_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(benchmark_benchmark_benchmark_main_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${benchmark_benchmark_benchmark_main_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${benchmark_benchmark_benchmark_main_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${benchmark_benchmark_benchmark_main_EXE_LINK_FLAGS_RELEASE}>
)
set(benchmark_benchmark_benchmark_main_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${benchmark_benchmark_benchmark_main_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${benchmark_benchmark_benchmark_main_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT benchmark::benchmark VARIABLES ############################################

set(benchmark_benchmark_benchmark_INCLUDE_DIRS_RELEASE "${benchmark_PACKAGE_FOLDER_RELEASE}/include")
set(benchmark_benchmark_benchmark_LIB_DIRS_RELEASE "${benchmark_PACKAGE_FOLDER_RELEASE}/lib")
set(benchmark_benchmark_benchmark_RES_DIRS_RELEASE )
set(benchmark_benchmark_benchmark_DEFINITIONS_RELEASE )
set(benchmark_benchmark_benchmark_OBJECTS_RELEASE )
set(benchmark_benchmark_benchmark_COMPILE_DEFINITIONS_RELEASE )
set(benchmark_benchmark_benchmark_COMPILE_OPTIONS_C_RELEASE "")
set(benchmark_benchmark_benchmark_COMPILE_OPTIONS_CXX_RELEASE "")
set(benchmark_benchmark_benchmark_LIBS_RELEASE benchmark)
set(benchmark_benchmark_benchmark_SYSTEM_LIBS_RELEASE )
set(benchmark_benchmark_benchmark_FRAMEWORK_DIRS_RELEASE )
set(benchmark_benchmark_benchmark_FRAMEWORKS_RELEASE )
set(benchmark_benchmark_benchmark_DEPENDENCIES_RELEASE )
set(benchmark_benchmark_benchmark_SHARED_LINK_FLAGS_RELEASE )
set(benchmark_benchmark_benchmark_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(benchmark_benchmark_benchmark_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${benchmark_benchmark_benchmark_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${benchmark_benchmark_benchmark_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${benchmark_benchmark_benchmark_EXE_LINK_FLAGS_RELEASE}>
)
set(benchmark_benchmark_benchmark_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${benchmark_benchmark_benchmark_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${benchmark_benchmark_benchmark_COMPILE_OPTIONS_C_RELEASE}>")