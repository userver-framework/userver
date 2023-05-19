########### AGGREGATED COMPONENTS AND DEPENDENCIES FOR THE MULTI CONFIG #####################
#############################################################################################

list(APPEND grpc_COMPONENT_NAMES gRPC::address_sorting gRPC::upb gRPC::grpc_plugin_support grpc::grpc_execs gRPC::gpr gRPC::grpc gRPC::grpc++ gRPC::grpc++_alts gRPC::grpc++_error_details gRPC::grpc_unsecure gRPC::grpc++_unsecure gRPC::grpc++_reflection gRPC::grpcpp_channelz)
list(REMOVE_DUPLICATES grpc_COMPONENT_NAMES)
list(APPEND grpc_FIND_DEPENDENCY_NAMES absl absl absl absl absl absl absl absl absl absl absl absl absl c-ares OpenSSL OpenSSL re2 ZLIB protobuf absl absl absl ZLIB absl grpc-proto googleapis)
list(REMOVE_DUPLICATES grpc_FIND_DEPENDENCY_NAMES)
set(absl_FIND_MODE "NO_MODULE")
set(c-ares_FIND_MODE "NO_MODULE")
set(OpenSSL_FIND_MODE "NO_MODULE")
set(re2_FIND_MODE "NO_MODULE")
set(ZLIB_FIND_MODE "NO_MODULE")
set(protobuf_FIND_MODE "NO_MODULE")
set(googleapis_FIND_MODE "NO_MODULE")
set(grpc-proto_FIND_MODE "NO_MODULE")

########### VARIABLES #######################################################################
#############################################################################################
set(grpc_PACKAGE_FOLDER_RELEASE "/Users/alexiprof/.conan/data/grpc/1.48.0/_/_/package/747f375e854e326b233e80494b3c63476474d77a")
set(grpc_BUILD_MODULES_PATHS_RELEASE "${grpc_PACKAGE_FOLDER_RELEASE}/lib/cmake/conan_trick/grpc_cpp_plugin.cmake"
			"${grpc_PACKAGE_FOLDER_RELEASE}/lib/cmake/conan_trick/grpc_csharp_plugin.cmake"
			"${grpc_PACKAGE_FOLDER_RELEASE}/lib/cmake/conan_trick/grpc_node_plugin.cmake"
			"${grpc_PACKAGE_FOLDER_RELEASE}/lib/cmake/conan_trick/grpc_objective_c_plugin.cmake"
			"${grpc_PACKAGE_FOLDER_RELEASE}/lib/cmake/conan_trick/grpc_php_plugin.cmake"
			"${grpc_PACKAGE_FOLDER_RELEASE}/lib/cmake/conan_trick/grpc_python_plugin.cmake"
			"${grpc_PACKAGE_FOLDER_RELEASE}/lib/cmake/conan_trick/grpc_ruby_plugin.cmake")


set(grpc_INCLUDE_DIRS_RELEASE "${grpc_PACKAGE_FOLDER_RELEASE}/include")
set(grpc_RES_DIRS_RELEASE )
set(grpc_DEFINITIONS_RELEASE )
set(grpc_SHARED_LINK_FLAGS_RELEASE )
set(grpc_EXE_LINK_FLAGS_RELEASE )
set(grpc_OBJECTS_RELEASE )
set(grpc_COMPILE_DEFINITIONS_RELEASE )
set(grpc_COMPILE_OPTIONS_C_RELEASE )
set(grpc_COMPILE_OPTIONS_CXX_RELEASE )
set(grpc_LIB_DIRS_RELEASE "${grpc_PACKAGE_FOLDER_RELEASE}/lib")
set(grpc_LIBS_RELEASE grpcpp_channelz grpc++_reflection grpc++_unsecure grpc_unsecure grpc++_error_details grpc++_alts grpc++ grpc gpr grpc_plugin_support upb address_sorting)
set(grpc_SYSTEM_LIBS_RELEASE )
set(grpc_FRAMEWORK_DIRS_RELEASE )
set(grpc_FRAMEWORKS_RELEASE CoreFoundation)
set(grpc_BUILD_DIRS_RELEASE )

# COMPOUND VARIABLES
set(grpc_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${grpc_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${grpc_COMPILE_OPTIONS_C_RELEASE}>")
set(grpc_LINKER_FLAGS_RELEASE
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${grpc_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${grpc_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${grpc_EXE_LINK_FLAGS_RELEASE}>")


set(grpc_COMPONENTS_RELEASE gRPC::address_sorting gRPC::upb gRPC::grpc_plugin_support grpc::grpc_execs gRPC::gpr gRPC::grpc gRPC::grpc++ gRPC::grpc++_alts gRPC::grpc++_error_details gRPC::grpc_unsecure gRPC::grpc++_unsecure gRPC::grpc++_reflection gRPC::grpcpp_channelz)
########### COMPONENT gRPC::grpcpp_channelz VARIABLES ############################################

set(grpc_gRPC_grpcpp_channelz_INCLUDE_DIRS_RELEASE "${grpc_PACKAGE_FOLDER_RELEASE}/include")
set(grpc_gRPC_grpcpp_channelz_LIB_DIRS_RELEASE "${grpc_PACKAGE_FOLDER_RELEASE}/lib")
set(grpc_gRPC_grpcpp_channelz_RES_DIRS_RELEASE )
set(grpc_gRPC_grpcpp_channelz_DEFINITIONS_RELEASE )
set(grpc_gRPC_grpcpp_channelz_OBJECTS_RELEASE )
set(grpc_gRPC_grpcpp_channelz_COMPILE_DEFINITIONS_RELEASE )
set(grpc_gRPC_grpcpp_channelz_COMPILE_OPTIONS_C_RELEASE "")
set(grpc_gRPC_grpcpp_channelz_COMPILE_OPTIONS_CXX_RELEASE "")
set(grpc_gRPC_grpcpp_channelz_LIBS_RELEASE grpcpp_channelz)
set(grpc_gRPC_grpcpp_channelz_SYSTEM_LIBS_RELEASE )
set(grpc_gRPC_grpcpp_channelz_FRAMEWORK_DIRS_RELEASE )
set(grpc_gRPC_grpcpp_channelz_FRAMEWORKS_RELEASE )
set(grpc_gRPC_grpcpp_channelz_DEPENDENCIES_RELEASE gRPC::grpc++ protobuf::libprotobuf grpc-proto::grpc-proto googleapis::googleapis)
set(grpc_gRPC_grpcpp_channelz_SHARED_LINK_FLAGS_RELEASE )
set(grpc_gRPC_grpcpp_channelz_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(grpc_gRPC_grpcpp_channelz_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${grpc_gRPC_grpcpp_channelz_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${grpc_gRPC_grpcpp_channelz_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${grpc_gRPC_grpcpp_channelz_EXE_LINK_FLAGS_RELEASE}>
)
set(grpc_gRPC_grpcpp_channelz_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${grpc_gRPC_grpcpp_channelz_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${grpc_gRPC_grpcpp_channelz_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT gRPC::grpc++_reflection VARIABLES ############################################

set(grpc_gRPC_grpc++_reflection_INCLUDE_DIRS_RELEASE "${grpc_PACKAGE_FOLDER_RELEASE}/include")
set(grpc_gRPC_grpc++_reflection_LIB_DIRS_RELEASE "${grpc_PACKAGE_FOLDER_RELEASE}/lib")
set(grpc_gRPC_grpc++_reflection_RES_DIRS_RELEASE )
set(grpc_gRPC_grpc++_reflection_DEFINITIONS_RELEASE )
set(grpc_gRPC_grpc++_reflection_OBJECTS_RELEASE )
set(grpc_gRPC_grpc++_reflection_COMPILE_DEFINITIONS_RELEASE )
set(grpc_gRPC_grpc++_reflection_COMPILE_OPTIONS_C_RELEASE "")
set(grpc_gRPC_grpc++_reflection_COMPILE_OPTIONS_CXX_RELEASE "")
set(grpc_gRPC_grpc++_reflection_LIBS_RELEASE grpc++_reflection)
set(grpc_gRPC_grpc++_reflection_SYSTEM_LIBS_RELEASE )
set(grpc_gRPC_grpc++_reflection_FRAMEWORK_DIRS_RELEASE )
set(grpc_gRPC_grpc++_reflection_FRAMEWORKS_RELEASE )
set(grpc_gRPC_grpc++_reflection_DEPENDENCIES_RELEASE gRPC::grpc++ protobuf::libprotobuf grpc-proto::grpc-proto googleapis::googleapis)
set(grpc_gRPC_grpc++_reflection_SHARED_LINK_FLAGS_RELEASE )
set(grpc_gRPC_grpc++_reflection_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(grpc_gRPC_grpc++_reflection_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${grpc_gRPC_grpc++_reflection_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${grpc_gRPC_grpc++_reflection_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${grpc_gRPC_grpc++_reflection_EXE_LINK_FLAGS_RELEASE}>
)
set(grpc_gRPC_grpc++_reflection_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${grpc_gRPC_grpc++_reflection_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${grpc_gRPC_grpc++_reflection_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT gRPC::grpc++_unsecure VARIABLES ############################################

set(grpc_gRPC_grpc++_unsecure_INCLUDE_DIRS_RELEASE "${grpc_PACKAGE_FOLDER_RELEASE}/include")
set(grpc_gRPC_grpc++_unsecure_LIB_DIRS_RELEASE "${grpc_PACKAGE_FOLDER_RELEASE}/lib")
set(grpc_gRPC_grpc++_unsecure_RES_DIRS_RELEASE )
set(grpc_gRPC_grpc++_unsecure_DEFINITIONS_RELEASE )
set(grpc_gRPC_grpc++_unsecure_OBJECTS_RELEASE )
set(grpc_gRPC_grpc++_unsecure_COMPILE_DEFINITIONS_RELEASE )
set(grpc_gRPC_grpc++_unsecure_COMPILE_OPTIONS_C_RELEASE "")
set(grpc_gRPC_grpc++_unsecure_COMPILE_OPTIONS_CXX_RELEASE "")
set(grpc_gRPC_grpc++_unsecure_LIBS_RELEASE grpc++_unsecure)
set(grpc_gRPC_grpc++_unsecure_SYSTEM_LIBS_RELEASE )
set(grpc_gRPC_grpc++_unsecure_FRAMEWORK_DIRS_RELEASE )
set(grpc_gRPC_grpc++_unsecure_FRAMEWORKS_RELEASE )
set(grpc_gRPC_grpc++_unsecure_DEPENDENCIES_RELEASE gRPC::grpc_unsecure protobuf::libprotobuf)
set(grpc_gRPC_grpc++_unsecure_SHARED_LINK_FLAGS_RELEASE )
set(grpc_gRPC_grpc++_unsecure_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(grpc_gRPC_grpc++_unsecure_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${grpc_gRPC_grpc++_unsecure_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${grpc_gRPC_grpc++_unsecure_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${grpc_gRPC_grpc++_unsecure_EXE_LINK_FLAGS_RELEASE}>
)
set(grpc_gRPC_grpc++_unsecure_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${grpc_gRPC_grpc++_unsecure_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${grpc_gRPC_grpc++_unsecure_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT gRPC::grpc_unsecure VARIABLES ############################################

set(grpc_gRPC_grpc_unsecure_INCLUDE_DIRS_RELEASE "${grpc_PACKAGE_FOLDER_RELEASE}/include")
set(grpc_gRPC_grpc_unsecure_LIB_DIRS_RELEASE "${grpc_PACKAGE_FOLDER_RELEASE}/lib")
set(grpc_gRPC_grpc_unsecure_RES_DIRS_RELEASE )
set(grpc_gRPC_grpc_unsecure_DEFINITIONS_RELEASE )
set(grpc_gRPC_grpc_unsecure_OBJECTS_RELEASE )
set(grpc_gRPC_grpc_unsecure_COMPILE_DEFINITIONS_RELEASE )
set(grpc_gRPC_grpc_unsecure_COMPILE_OPTIONS_C_RELEASE "")
set(grpc_gRPC_grpc_unsecure_COMPILE_OPTIONS_CXX_RELEASE "")
set(grpc_gRPC_grpc_unsecure_LIBS_RELEASE grpc_unsecure)
set(grpc_gRPC_grpc_unsecure_SYSTEM_LIBS_RELEASE )
set(grpc_gRPC_grpc_unsecure_FRAMEWORK_DIRS_RELEASE )
set(grpc_gRPC_grpc_unsecure_FRAMEWORKS_RELEASE CoreFoundation)
set(grpc_gRPC_grpc_unsecure_DEPENDENCIES_RELEASE gRPC::address_sorting gRPC::gpr gRPC::upb absl::flat_hash_map absl::inlined_vector absl::statusor c-ares::cares re2::re2 ZLIB::ZLIB absl::random_random)
set(grpc_gRPC_grpc_unsecure_SHARED_LINK_FLAGS_RELEASE )
set(grpc_gRPC_grpc_unsecure_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(grpc_gRPC_grpc_unsecure_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${grpc_gRPC_grpc_unsecure_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${grpc_gRPC_grpc_unsecure_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${grpc_gRPC_grpc_unsecure_EXE_LINK_FLAGS_RELEASE}>
)
set(grpc_gRPC_grpc_unsecure_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${grpc_gRPC_grpc_unsecure_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${grpc_gRPC_grpc_unsecure_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT gRPC::grpc++_error_details VARIABLES ############################################

set(grpc_gRPC_grpc++_error_details_INCLUDE_DIRS_RELEASE "${grpc_PACKAGE_FOLDER_RELEASE}/include")
set(grpc_gRPC_grpc++_error_details_LIB_DIRS_RELEASE "${grpc_PACKAGE_FOLDER_RELEASE}/lib")
set(grpc_gRPC_grpc++_error_details_RES_DIRS_RELEASE )
set(grpc_gRPC_grpc++_error_details_DEFINITIONS_RELEASE )
set(grpc_gRPC_grpc++_error_details_OBJECTS_RELEASE )
set(grpc_gRPC_grpc++_error_details_COMPILE_DEFINITIONS_RELEASE )
set(grpc_gRPC_grpc++_error_details_COMPILE_OPTIONS_C_RELEASE "")
set(grpc_gRPC_grpc++_error_details_COMPILE_OPTIONS_CXX_RELEASE "")
set(grpc_gRPC_grpc++_error_details_LIBS_RELEASE grpc++_error_details)
set(grpc_gRPC_grpc++_error_details_SYSTEM_LIBS_RELEASE )
set(grpc_gRPC_grpc++_error_details_FRAMEWORK_DIRS_RELEASE )
set(grpc_gRPC_grpc++_error_details_FRAMEWORKS_RELEASE )
set(grpc_gRPC_grpc++_error_details_DEPENDENCIES_RELEASE gRPC::grpc++ protobuf::libprotobuf)
set(grpc_gRPC_grpc++_error_details_SHARED_LINK_FLAGS_RELEASE )
set(grpc_gRPC_grpc++_error_details_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(grpc_gRPC_grpc++_error_details_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${grpc_gRPC_grpc++_error_details_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${grpc_gRPC_grpc++_error_details_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${grpc_gRPC_grpc++_error_details_EXE_LINK_FLAGS_RELEASE}>
)
set(grpc_gRPC_grpc++_error_details_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${grpc_gRPC_grpc++_error_details_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${grpc_gRPC_grpc++_error_details_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT gRPC::grpc++_alts VARIABLES ############################################

set(grpc_gRPC_grpc++_alts_INCLUDE_DIRS_RELEASE "${grpc_PACKAGE_FOLDER_RELEASE}/include")
set(grpc_gRPC_grpc++_alts_LIB_DIRS_RELEASE "${grpc_PACKAGE_FOLDER_RELEASE}/lib")
set(grpc_gRPC_grpc++_alts_RES_DIRS_RELEASE )
set(grpc_gRPC_grpc++_alts_DEFINITIONS_RELEASE )
set(grpc_gRPC_grpc++_alts_OBJECTS_RELEASE )
set(grpc_gRPC_grpc++_alts_COMPILE_DEFINITIONS_RELEASE )
set(grpc_gRPC_grpc++_alts_COMPILE_OPTIONS_C_RELEASE "")
set(grpc_gRPC_grpc++_alts_COMPILE_OPTIONS_CXX_RELEASE "")
set(grpc_gRPC_grpc++_alts_LIBS_RELEASE grpc++_alts)
set(grpc_gRPC_grpc++_alts_SYSTEM_LIBS_RELEASE )
set(grpc_gRPC_grpc++_alts_FRAMEWORK_DIRS_RELEASE )
set(grpc_gRPC_grpc++_alts_FRAMEWORKS_RELEASE )
set(grpc_gRPC_grpc++_alts_DEPENDENCIES_RELEASE gRPC::grpc++ protobuf::libprotobuf)
set(grpc_gRPC_grpc++_alts_SHARED_LINK_FLAGS_RELEASE )
set(grpc_gRPC_grpc++_alts_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(grpc_gRPC_grpc++_alts_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${grpc_gRPC_grpc++_alts_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${grpc_gRPC_grpc++_alts_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${grpc_gRPC_grpc++_alts_EXE_LINK_FLAGS_RELEASE}>
)
set(grpc_gRPC_grpc++_alts_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${grpc_gRPC_grpc++_alts_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${grpc_gRPC_grpc++_alts_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT gRPC::grpc++ VARIABLES ############################################

set(grpc_gRPC_grpc++_INCLUDE_DIRS_RELEASE "${grpc_PACKAGE_FOLDER_RELEASE}/include")
set(grpc_gRPC_grpc++_LIB_DIRS_RELEASE "${grpc_PACKAGE_FOLDER_RELEASE}/lib")
set(grpc_gRPC_grpc++_RES_DIRS_RELEASE )
set(grpc_gRPC_grpc++_DEFINITIONS_RELEASE )
set(grpc_gRPC_grpc++_OBJECTS_RELEASE )
set(grpc_gRPC_grpc++_COMPILE_DEFINITIONS_RELEASE )
set(grpc_gRPC_grpc++_COMPILE_OPTIONS_C_RELEASE "")
set(grpc_gRPC_grpc++_COMPILE_OPTIONS_CXX_RELEASE "")
set(grpc_gRPC_grpc++_LIBS_RELEASE grpc++)
set(grpc_gRPC_grpc++_SYSTEM_LIBS_RELEASE )
set(grpc_gRPC_grpc++_FRAMEWORK_DIRS_RELEASE )
set(grpc_gRPC_grpc++_FRAMEWORKS_RELEASE )
set(grpc_gRPC_grpc++_DEPENDENCIES_RELEASE gRPC::grpc protobuf::libprotobuf)
set(grpc_gRPC_grpc++_SHARED_LINK_FLAGS_RELEASE )
set(grpc_gRPC_grpc++_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(grpc_gRPC_grpc++_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${grpc_gRPC_grpc++_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${grpc_gRPC_grpc++_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${grpc_gRPC_grpc++_EXE_LINK_FLAGS_RELEASE}>
)
set(grpc_gRPC_grpc++_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${grpc_gRPC_grpc++_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${grpc_gRPC_grpc++_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT gRPC::grpc VARIABLES ############################################

set(grpc_gRPC_grpc_INCLUDE_DIRS_RELEASE "${grpc_PACKAGE_FOLDER_RELEASE}/include")
set(grpc_gRPC_grpc_LIB_DIRS_RELEASE "${grpc_PACKAGE_FOLDER_RELEASE}/lib")
set(grpc_gRPC_grpc_RES_DIRS_RELEASE )
set(grpc_gRPC_grpc_DEFINITIONS_RELEASE )
set(grpc_gRPC_grpc_OBJECTS_RELEASE )
set(grpc_gRPC_grpc_COMPILE_DEFINITIONS_RELEASE )
set(grpc_gRPC_grpc_COMPILE_OPTIONS_C_RELEASE "")
set(grpc_gRPC_grpc_COMPILE_OPTIONS_CXX_RELEASE "")
set(grpc_gRPC_grpc_LIBS_RELEASE grpc)
set(grpc_gRPC_grpc_SYSTEM_LIBS_RELEASE )
set(grpc_gRPC_grpc_FRAMEWORK_DIRS_RELEASE )
set(grpc_gRPC_grpc_FRAMEWORKS_RELEASE CoreFoundation)
set(grpc_gRPC_grpc_DEPENDENCIES_RELEASE gRPC::address_sorting gRPC::gpr gRPC::upb absl::bind_front absl::flat_hash_map absl::inlined_vector absl::statusor absl::random_random c-ares::cares OpenSSL::Crypto OpenSSL::SSL re2::re2 ZLIB::ZLIB)
set(grpc_gRPC_grpc_SHARED_LINK_FLAGS_RELEASE )
set(grpc_gRPC_grpc_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(grpc_gRPC_grpc_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${grpc_gRPC_grpc_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${grpc_gRPC_grpc_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${grpc_gRPC_grpc_EXE_LINK_FLAGS_RELEASE}>
)
set(grpc_gRPC_grpc_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${grpc_gRPC_grpc_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${grpc_gRPC_grpc_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT gRPC::gpr VARIABLES ############################################

set(grpc_gRPC_gpr_INCLUDE_DIRS_RELEASE "${grpc_PACKAGE_FOLDER_RELEASE}/include")
set(grpc_gRPC_gpr_LIB_DIRS_RELEASE "${grpc_PACKAGE_FOLDER_RELEASE}/lib")
set(grpc_gRPC_gpr_RES_DIRS_RELEASE )
set(grpc_gRPC_gpr_DEFINITIONS_RELEASE )
set(grpc_gRPC_gpr_OBJECTS_RELEASE )
set(grpc_gRPC_gpr_COMPILE_DEFINITIONS_RELEASE )
set(grpc_gRPC_gpr_COMPILE_OPTIONS_C_RELEASE "")
set(grpc_gRPC_gpr_COMPILE_OPTIONS_CXX_RELEASE "")
set(grpc_gRPC_gpr_LIBS_RELEASE gpr)
set(grpc_gRPC_gpr_SYSTEM_LIBS_RELEASE )
set(grpc_gRPC_gpr_FRAMEWORK_DIRS_RELEASE )
set(grpc_gRPC_gpr_FRAMEWORKS_RELEASE )
set(grpc_gRPC_gpr_DEPENDENCIES_RELEASE gRPC::upb absl::base absl::memory absl::status absl::str_format absl::strings absl::synchronization absl::time absl::optional)
set(grpc_gRPC_gpr_SHARED_LINK_FLAGS_RELEASE )
set(grpc_gRPC_gpr_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(grpc_gRPC_gpr_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${grpc_gRPC_gpr_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${grpc_gRPC_gpr_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${grpc_gRPC_gpr_EXE_LINK_FLAGS_RELEASE}>
)
set(grpc_gRPC_gpr_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${grpc_gRPC_gpr_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${grpc_gRPC_gpr_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT grpc::grpc_execs VARIABLES ############################################

set(grpc_grpc_grpc_execs_INCLUDE_DIRS_RELEASE "${grpc_PACKAGE_FOLDER_RELEASE}/include")
set(grpc_grpc_grpc_execs_LIB_DIRS_RELEASE "${grpc_PACKAGE_FOLDER_RELEASE}/lib")
set(grpc_grpc_grpc_execs_RES_DIRS_RELEASE )
set(grpc_grpc_grpc_execs_DEFINITIONS_RELEASE )
set(grpc_grpc_grpc_execs_OBJECTS_RELEASE )
set(grpc_grpc_grpc_execs_COMPILE_DEFINITIONS_RELEASE )
set(grpc_grpc_grpc_execs_COMPILE_OPTIONS_C_RELEASE "")
set(grpc_grpc_grpc_execs_COMPILE_OPTIONS_CXX_RELEASE "")
set(grpc_grpc_grpc_execs_LIBS_RELEASE )
set(grpc_grpc_grpc_execs_SYSTEM_LIBS_RELEASE )
set(grpc_grpc_grpc_execs_FRAMEWORK_DIRS_RELEASE )
set(grpc_grpc_grpc_execs_FRAMEWORKS_RELEASE )
set(grpc_grpc_grpc_execs_DEPENDENCIES_RELEASE )
set(grpc_grpc_grpc_execs_SHARED_LINK_FLAGS_RELEASE )
set(grpc_grpc_grpc_execs_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(grpc_grpc_grpc_execs_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${grpc_grpc_grpc_execs_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${grpc_grpc_grpc_execs_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${grpc_grpc_grpc_execs_EXE_LINK_FLAGS_RELEASE}>
)
set(grpc_grpc_grpc_execs_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${grpc_grpc_grpc_execs_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${grpc_grpc_grpc_execs_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT gRPC::grpc_plugin_support VARIABLES ############################################

set(grpc_gRPC_grpc_plugin_support_INCLUDE_DIRS_RELEASE "${grpc_PACKAGE_FOLDER_RELEASE}/include")
set(grpc_gRPC_grpc_plugin_support_LIB_DIRS_RELEASE "${grpc_PACKAGE_FOLDER_RELEASE}/lib")
set(grpc_gRPC_grpc_plugin_support_RES_DIRS_RELEASE )
set(grpc_gRPC_grpc_plugin_support_DEFINITIONS_RELEASE )
set(grpc_gRPC_grpc_plugin_support_OBJECTS_RELEASE )
set(grpc_gRPC_grpc_plugin_support_COMPILE_DEFINITIONS_RELEASE )
set(grpc_gRPC_grpc_plugin_support_COMPILE_OPTIONS_C_RELEASE "")
set(grpc_gRPC_grpc_plugin_support_COMPILE_OPTIONS_CXX_RELEASE "")
set(grpc_gRPC_grpc_plugin_support_LIBS_RELEASE grpc_plugin_support)
set(grpc_gRPC_grpc_plugin_support_SYSTEM_LIBS_RELEASE )
set(grpc_gRPC_grpc_plugin_support_FRAMEWORK_DIRS_RELEASE )
set(grpc_gRPC_grpc_plugin_support_FRAMEWORKS_RELEASE )
set(grpc_gRPC_grpc_plugin_support_DEPENDENCIES_RELEASE protobuf::libprotoc protobuf::libprotobuf)
set(grpc_gRPC_grpc_plugin_support_SHARED_LINK_FLAGS_RELEASE )
set(grpc_gRPC_grpc_plugin_support_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(grpc_gRPC_grpc_plugin_support_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${grpc_gRPC_grpc_plugin_support_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${grpc_gRPC_grpc_plugin_support_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${grpc_gRPC_grpc_plugin_support_EXE_LINK_FLAGS_RELEASE}>
)
set(grpc_gRPC_grpc_plugin_support_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${grpc_gRPC_grpc_plugin_support_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${grpc_gRPC_grpc_plugin_support_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT gRPC::upb VARIABLES ############################################

set(grpc_gRPC_upb_INCLUDE_DIRS_RELEASE "${grpc_PACKAGE_FOLDER_RELEASE}/include")
set(grpc_gRPC_upb_LIB_DIRS_RELEASE "${grpc_PACKAGE_FOLDER_RELEASE}/lib")
set(grpc_gRPC_upb_RES_DIRS_RELEASE )
set(grpc_gRPC_upb_DEFINITIONS_RELEASE )
set(grpc_gRPC_upb_OBJECTS_RELEASE )
set(grpc_gRPC_upb_COMPILE_DEFINITIONS_RELEASE )
set(grpc_gRPC_upb_COMPILE_OPTIONS_C_RELEASE "")
set(grpc_gRPC_upb_COMPILE_OPTIONS_CXX_RELEASE "")
set(grpc_gRPC_upb_LIBS_RELEASE upb)
set(grpc_gRPC_upb_SYSTEM_LIBS_RELEASE )
set(grpc_gRPC_upb_FRAMEWORK_DIRS_RELEASE )
set(grpc_gRPC_upb_FRAMEWORKS_RELEASE )
set(grpc_gRPC_upb_DEPENDENCIES_RELEASE )
set(grpc_gRPC_upb_SHARED_LINK_FLAGS_RELEASE )
set(grpc_gRPC_upb_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(grpc_gRPC_upb_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${grpc_gRPC_upb_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${grpc_gRPC_upb_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${grpc_gRPC_upb_EXE_LINK_FLAGS_RELEASE}>
)
set(grpc_gRPC_upb_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${grpc_gRPC_upb_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${grpc_gRPC_upb_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT gRPC::address_sorting VARIABLES ############################################

set(grpc_gRPC_address_sorting_INCLUDE_DIRS_RELEASE "${grpc_PACKAGE_FOLDER_RELEASE}/include")
set(grpc_gRPC_address_sorting_LIB_DIRS_RELEASE "${grpc_PACKAGE_FOLDER_RELEASE}/lib")
set(grpc_gRPC_address_sorting_RES_DIRS_RELEASE )
set(grpc_gRPC_address_sorting_DEFINITIONS_RELEASE )
set(grpc_gRPC_address_sorting_OBJECTS_RELEASE )
set(grpc_gRPC_address_sorting_COMPILE_DEFINITIONS_RELEASE )
set(grpc_gRPC_address_sorting_COMPILE_OPTIONS_C_RELEASE "")
set(grpc_gRPC_address_sorting_COMPILE_OPTIONS_CXX_RELEASE "")
set(grpc_gRPC_address_sorting_LIBS_RELEASE address_sorting)
set(grpc_gRPC_address_sorting_SYSTEM_LIBS_RELEASE )
set(grpc_gRPC_address_sorting_FRAMEWORK_DIRS_RELEASE )
set(grpc_gRPC_address_sorting_FRAMEWORKS_RELEASE )
set(grpc_gRPC_address_sorting_DEPENDENCIES_RELEASE )
set(grpc_gRPC_address_sorting_SHARED_LINK_FLAGS_RELEASE )
set(grpc_gRPC_address_sorting_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(grpc_gRPC_address_sorting_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${grpc_gRPC_address_sorting_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${grpc_gRPC_address_sorting_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${grpc_gRPC_address_sorting_EXE_LINK_FLAGS_RELEASE}>
)
set(grpc_gRPC_address_sorting_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${grpc_gRPC_address_sorting_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${grpc_gRPC_address_sorting_COMPILE_OPTIONS_C_RELEASE}>")