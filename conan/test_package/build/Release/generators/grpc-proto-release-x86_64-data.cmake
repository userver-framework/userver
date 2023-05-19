########### AGGREGATED COMPONENTS AND DEPENDENCIES FOR THE MULTI CONFIG #####################
#############################################################################################

set(grpc-proto_COMPONENT_NAMES "")
list(APPEND grpc-proto_FIND_DEPENDENCY_NAMES protobuf googleapis)
list(REMOVE_DUPLICATES grpc-proto_FIND_DEPENDENCY_NAMES)
set(protobuf_FIND_MODE "NO_MODULE")
set(googleapis_FIND_MODE "NO_MODULE")

########### VARIABLES #######################################################################
#############################################################################################
set(grpc-proto_PACKAGE_FOLDER_RELEASE "/Users/alexiprof/.conan/data/grpc-proto/cci.20220627/_/_/package/4b7ed5be66f388b91704f85ba054bc3a1947e5f0")
set(grpc-proto_BUILD_MODULES_PATHS_RELEASE )


set(grpc-proto_INCLUDE_DIRS_RELEASE "${grpc-proto_PACKAGE_FOLDER_RELEASE}/include")
set(grpc-proto_RES_DIRS_RELEASE "${grpc-proto_PACKAGE_FOLDER_RELEASE}/res")
set(grpc-proto_DEFINITIONS_RELEASE )
set(grpc-proto_SHARED_LINK_FLAGS_RELEASE )
set(grpc-proto_EXE_LINK_FLAGS_RELEASE )
set(grpc-proto_OBJECTS_RELEASE )
set(grpc-proto_COMPILE_DEFINITIONS_RELEASE )
set(grpc-proto_COMPILE_OPTIONS_C_RELEASE )
set(grpc-proto_COMPILE_OPTIONS_CXX_RELEASE )
set(grpc-proto_LIB_DIRS_RELEASE "${grpc-proto_PACKAGE_FOLDER_RELEASE}/lib")
set(grpc-proto_LIBS_RELEASE grpc_alts_handshaker_proto grpc_benchmark_service_proto grpc_binarylog_proto grpc_channelz_proto grpc_control_proto grpc_grpclb_load_balancer_proto grpc_grpclb_load_reporter_proto grpc_health_proto grpc_messages_proto grpc_reflection_proto grpc_reflection_proto_deprecated grpc_report_qps_scenario_service_proto grpc_rls_config_proto grpc_rls_proto grpc_service_config_proto grpc_stats_proto grpc_worker_service_proto)
set(grpc-proto_SYSTEM_LIBS_RELEASE )
set(grpc-proto_FRAMEWORK_DIRS_RELEASE "${grpc-proto_PACKAGE_FOLDER_RELEASE}/Frameworks")
set(grpc-proto_FRAMEWORKS_RELEASE )
set(grpc-proto_BUILD_DIRS_RELEASE "${grpc-proto_PACKAGE_FOLDER_RELEASE}/")

# COMPOUND VARIABLES
set(grpc-proto_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${grpc-proto_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${grpc-proto_COMPILE_OPTIONS_C_RELEASE}>")
set(grpc-proto_LINKER_FLAGS_RELEASE
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${grpc-proto_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${grpc-proto_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${grpc-proto_EXE_LINK_FLAGS_RELEASE}>")


set(grpc-proto_COMPONENTS_RELEASE )