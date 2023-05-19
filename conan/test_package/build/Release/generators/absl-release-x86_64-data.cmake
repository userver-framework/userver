########### AGGREGATED COMPONENTS AND DEPENDENCIES FOR THE MULTI CONFIG #####################
#############################################################################################

list(APPEND abseil_COMPONENT_NAMES absl::config absl::dynamic_annotations absl::core_headers absl::pretty_function absl::fast_type_id absl::prefetch absl::algorithm absl::counting_allocator absl::hashtable_debug_hooks absl::node_slot_policy absl::leak_check absl::flags_commandlineflag_internal absl::type_traits absl::meta absl::bits absl::int128 absl::numeric absl::numeric_representation absl::exponential_biased absl::periodic_sampler absl::random_seed_gen_exception absl::random_internal_traits absl::random_internal_fast_uniform_bits absl::random_internal_iostream_state_saver absl::random_internal_wide_multiply absl::random_internal_fastmath absl::random_internal_pcg_engine absl::random_internal_platform absl::random_internal_randen_slow absl::random_internal_randen_hwaes_impl absl::random_internal_uniform_helper absl::cordz_update_tracker absl::civil_time absl::time_zone absl::compare absl::atomic_hook absl::errno_saver absl::log_severity absl::raw_logging_internal absl::base_internal absl::throw_delegate absl::scoped_set_env absl::strerror absl::algorithm_container absl::hash_policy_traits absl::hashtable_debug absl::container_common absl::debugging_internal absl::function_ref absl::memory absl::random_internal_generate_real absl::random_internal_randen_hwaes absl::cordz_functions absl::bad_any_cast_impl absl::span absl::bad_optional_access absl::bad_variant_access absl::utility absl::spinlock_wait absl::base absl::endian absl::cleanup_internal absl::cleanup absl::compressed_tuple absl::fixed_array absl::inlined_vector_internal absl::inlined_vector absl::container_memory absl::stacktrace absl::demangle_internal absl::debugging absl::any_invocable absl::bind_front absl::city absl::low_level_hash absl::random_internal_distribution_caller absl::random_internal_randen absl::strings_internal absl::bad_any_cast absl::optional absl::variant absl::malloc_internal absl::random_bit_gen_ref absl::random_internal_mock_helpers absl::random_internal_randen_engine absl::strings absl::str_format_internal absl::graphcycles_internal absl::time absl::any absl::layout absl::symbolize absl::examine_stack absl::failure_signal_handler absl::flags_path_util absl::flags_commandlineflag absl::flags_private_handle_accessor absl::hash absl::random_distributions absl::random_internal_seed_material absl::random_internal_pool_urbg absl::random_internal_salted_seed_seq absl::random_internal_nonsecure_base absl::str_format absl::cord_internal absl::kernel_timeout_internal absl::synchronization absl::flags_program_name absl::flags_config absl::flags_marshalling absl::flags_internal absl::sample_recorder absl::random_seed_sequences absl::random_internal_distribution_test_util absl::cordz_statistics absl::cordz_handle absl::cordz_info absl::cordz_sample_token absl::cordz_update_scope absl::cord absl::btree absl::hash_function_defaults absl::hashtablez_sampler absl::raw_hash_set absl::random_random absl::status absl::statusor absl::flat_hash_set absl::node_hash_set absl::raw_hash_map absl::flat_hash_map absl::node_hash_map absl::flags_reflection absl::flags absl::flags_usage_internal absl::flags_usage absl::flags_parse)
list(REMOVE_DUPLICATES abseil_COMPONENT_NAMES)
set(abseil_FIND_DEPENDENCY_NAMES "")

########### VARIABLES #######################################################################
#############################################################################################
set(abseil_PACKAGE_FOLDER_RELEASE "/Users/alexiprof/.conan/data/abseil/20220623.0/_/_/package/3782cb8c3754bcc4f0ca2283b503d86479835458")
set(abseil_BUILD_MODULES_PATHS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib/cmake/conan_trick/cxx_std.cmake")


set(abseil_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_RES_DIRS_RELEASE )
set(abseil_DEFINITIONS_RELEASE )
set(abseil_SHARED_LINK_FLAGS_RELEASE )
set(abseil_EXE_LINK_FLAGS_RELEASE )
set(abseil_OBJECTS_RELEASE )
set(abseil_COMPILE_DEFINITIONS_RELEASE )
set(abseil_COMPILE_OPTIONS_C_RELEASE )
set(abseil_COMPILE_OPTIONS_CXX_RELEASE )
set(abseil_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_LIBS_RELEASE absl_flags_parse absl_flags_usage absl_flags_usage_internal absl_flags absl_flags_reflection absl_statusor absl_status absl_raw_hash_set absl_hashtablez_sampler absl_cord absl_cordz_sample_token absl_cordz_info absl_cordz_handle absl_random_internal_distribution_test_util absl_random_seed_sequences absl_flags_internal absl_flags_marshalling absl_flags_config absl_flags_program_name absl_synchronization absl_cord_internal absl_random_internal_pool_urbg absl_random_internal_seed_material absl_random_distributions absl_hash absl_flags_private_handle_accessor absl_flags_commandlineflag absl_failure_signal_handler absl_examine_stack absl_symbolize absl_time absl_graphcycles_internal absl_str_format_internal absl_strings absl_malloc_internal absl_strings_internal absl_random_internal_randen absl_low_level_hash absl_city absl_demangle_internal absl_stacktrace absl_base absl_spinlock_wait absl_bad_variant_access absl_bad_optional_access absl_bad_any_cast_impl absl_cordz_functions absl_random_internal_randen_hwaes absl_debugging_internal absl_strerror absl_scoped_set_env absl_throw_delegate absl_raw_logging_internal absl_log_severity absl_time_zone absl_civil_time absl_random_internal_randen_hwaes_impl absl_random_internal_randen_slow absl_random_internal_platform absl_random_seed_gen_exception absl_periodic_sampler absl_exponential_biased absl_int128 absl_flags_commandlineflag_internal absl_leak_check)
set(abseil_SYSTEM_LIBS_RELEASE )
set(abseil_FRAMEWORK_DIRS_RELEASE )
set(abseil_FRAMEWORKS_RELEASE CoreFoundation)
set(abseil_BUILD_DIRS_RELEASE )

# COMPOUND VARIABLES
set(abseil_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_COMPILE_OPTIONS_C_RELEASE}>")
set(abseil_LINKER_FLAGS_RELEASE
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_EXE_LINK_FLAGS_RELEASE}>")


set(abseil_COMPONENTS_RELEASE absl::config absl::dynamic_annotations absl::core_headers absl::pretty_function absl::fast_type_id absl::prefetch absl::algorithm absl::counting_allocator absl::hashtable_debug_hooks absl::node_slot_policy absl::leak_check absl::flags_commandlineflag_internal absl::type_traits absl::meta absl::bits absl::int128 absl::numeric absl::numeric_representation absl::exponential_biased absl::periodic_sampler absl::random_seed_gen_exception absl::random_internal_traits absl::random_internal_fast_uniform_bits absl::random_internal_iostream_state_saver absl::random_internal_wide_multiply absl::random_internal_fastmath absl::random_internal_pcg_engine absl::random_internal_platform absl::random_internal_randen_slow absl::random_internal_randen_hwaes_impl absl::random_internal_uniform_helper absl::cordz_update_tracker absl::civil_time absl::time_zone absl::compare absl::atomic_hook absl::errno_saver absl::log_severity absl::raw_logging_internal absl::base_internal absl::throw_delegate absl::scoped_set_env absl::strerror absl::algorithm_container absl::hash_policy_traits absl::hashtable_debug absl::container_common absl::debugging_internal absl::function_ref absl::memory absl::random_internal_generate_real absl::random_internal_randen_hwaes absl::cordz_functions absl::bad_any_cast_impl absl::span absl::bad_optional_access absl::bad_variant_access absl::utility absl::spinlock_wait absl::base absl::endian absl::cleanup_internal absl::cleanup absl::compressed_tuple absl::fixed_array absl::inlined_vector_internal absl::inlined_vector absl::container_memory absl::stacktrace absl::demangle_internal absl::debugging absl::any_invocable absl::bind_front absl::city absl::low_level_hash absl::random_internal_distribution_caller absl::random_internal_randen absl::strings_internal absl::bad_any_cast absl::optional absl::variant absl::malloc_internal absl::random_bit_gen_ref absl::random_internal_mock_helpers absl::random_internal_randen_engine absl::strings absl::str_format_internal absl::graphcycles_internal absl::time absl::any absl::layout absl::symbolize absl::examine_stack absl::failure_signal_handler absl::flags_path_util absl::flags_commandlineflag absl::flags_private_handle_accessor absl::hash absl::random_distributions absl::random_internal_seed_material absl::random_internal_pool_urbg absl::random_internal_salted_seed_seq absl::random_internal_nonsecure_base absl::str_format absl::cord_internal absl::kernel_timeout_internal absl::synchronization absl::flags_program_name absl::flags_config absl::flags_marshalling absl::flags_internal absl::sample_recorder absl::random_seed_sequences absl::random_internal_distribution_test_util absl::cordz_statistics absl::cordz_handle absl::cordz_info absl::cordz_sample_token absl::cordz_update_scope absl::cord absl::btree absl::hash_function_defaults absl::hashtablez_sampler absl::raw_hash_set absl::random_random absl::status absl::statusor absl::flat_hash_set absl::node_hash_set absl::raw_hash_map absl::flat_hash_map absl::node_hash_map absl::flags_reflection absl::flags absl::flags_usage_internal absl::flags_usage absl::flags_parse)
########### COMPONENT absl::flags_parse VARIABLES ############################################

set(abseil_absl_flags_parse_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_flags_parse_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_flags_parse_RES_DIRS_RELEASE )
set(abseil_absl_flags_parse_DEFINITIONS_RELEASE )
set(abseil_absl_flags_parse_OBJECTS_RELEASE )
set(abseil_absl_flags_parse_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_flags_parse_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_flags_parse_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_flags_parse_LIBS_RELEASE absl_flags_parse)
set(abseil_absl_flags_parse_SYSTEM_LIBS_RELEASE )
set(abseil_absl_flags_parse_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_flags_parse_FRAMEWORKS_RELEASE )
set(abseil_absl_flags_parse_DEPENDENCIES_RELEASE absl::config absl::core_headers absl::flags_config absl::flags absl::flags_commandlineflag absl::flags_commandlineflag_internal absl::flags_internal absl::flags_private_handle_accessor absl::flags_program_name absl::flags_reflection absl::flags_usage absl::strings absl::synchronization)
set(abseil_absl_flags_parse_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_flags_parse_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_flags_parse_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_flags_parse_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_flags_parse_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_flags_parse_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_flags_parse_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_flags_parse_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_flags_parse_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::flags_usage VARIABLES ############################################

set(abseil_absl_flags_usage_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_flags_usage_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_flags_usage_RES_DIRS_RELEASE )
set(abseil_absl_flags_usage_DEFINITIONS_RELEASE )
set(abseil_absl_flags_usage_OBJECTS_RELEASE )
set(abseil_absl_flags_usage_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_flags_usage_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_flags_usage_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_flags_usage_LIBS_RELEASE absl_flags_usage)
set(abseil_absl_flags_usage_SYSTEM_LIBS_RELEASE )
set(abseil_absl_flags_usage_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_flags_usage_FRAMEWORKS_RELEASE )
set(abseil_absl_flags_usage_DEPENDENCIES_RELEASE absl::config absl::core_headers absl::flags_usage_internal absl::strings absl::synchronization)
set(abseil_absl_flags_usage_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_flags_usage_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_flags_usage_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_flags_usage_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_flags_usage_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_flags_usage_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_flags_usage_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_flags_usage_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_flags_usage_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::flags_usage_internal VARIABLES ############################################

set(abseil_absl_flags_usage_internal_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_flags_usage_internal_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_flags_usage_internal_RES_DIRS_RELEASE )
set(abseil_absl_flags_usage_internal_DEFINITIONS_RELEASE )
set(abseil_absl_flags_usage_internal_OBJECTS_RELEASE )
set(abseil_absl_flags_usage_internal_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_flags_usage_internal_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_flags_usage_internal_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_flags_usage_internal_LIBS_RELEASE absl_flags_usage_internal)
set(abseil_absl_flags_usage_internal_SYSTEM_LIBS_RELEASE )
set(abseil_absl_flags_usage_internal_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_flags_usage_internal_FRAMEWORKS_RELEASE )
set(abseil_absl_flags_usage_internal_DEPENDENCIES_RELEASE absl::config absl::flags_config absl::flags absl::flags_commandlineflag absl::flags_internal absl::flags_path_util absl::flags_private_handle_accessor absl::flags_program_name absl::flags_reflection absl::flat_hash_map absl::strings absl::synchronization)
set(abseil_absl_flags_usage_internal_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_flags_usage_internal_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_flags_usage_internal_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_flags_usage_internal_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_flags_usage_internal_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_flags_usage_internal_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_flags_usage_internal_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_flags_usage_internal_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_flags_usage_internal_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::flags VARIABLES ############################################

set(abseil_absl_flags_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_flags_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_flags_RES_DIRS_RELEASE )
set(abseil_absl_flags_DEFINITIONS_RELEASE )
set(abseil_absl_flags_OBJECTS_RELEASE )
set(abseil_absl_flags_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_flags_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_flags_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_flags_LIBS_RELEASE absl_flags)
set(abseil_absl_flags_SYSTEM_LIBS_RELEASE )
set(abseil_absl_flags_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_flags_FRAMEWORKS_RELEASE )
set(abseil_absl_flags_DEPENDENCIES_RELEASE absl::config absl::flags_commandlineflag absl::flags_config absl::flags_internal absl::flags_reflection absl::base absl::core_headers absl::strings)
set(abseil_absl_flags_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_flags_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_flags_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_flags_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_flags_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_flags_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_flags_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_flags_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_flags_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::flags_reflection VARIABLES ############################################

set(abseil_absl_flags_reflection_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_flags_reflection_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_flags_reflection_RES_DIRS_RELEASE )
set(abseil_absl_flags_reflection_DEFINITIONS_RELEASE )
set(abseil_absl_flags_reflection_OBJECTS_RELEASE )
set(abseil_absl_flags_reflection_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_flags_reflection_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_flags_reflection_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_flags_reflection_LIBS_RELEASE absl_flags_reflection)
set(abseil_absl_flags_reflection_SYSTEM_LIBS_RELEASE )
set(abseil_absl_flags_reflection_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_flags_reflection_FRAMEWORKS_RELEASE )
set(abseil_absl_flags_reflection_DEPENDENCIES_RELEASE absl::config absl::flags_commandlineflag absl::flags_private_handle_accessor absl::flags_config absl::strings absl::synchronization absl::flat_hash_map)
set(abseil_absl_flags_reflection_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_flags_reflection_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_flags_reflection_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_flags_reflection_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_flags_reflection_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_flags_reflection_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_flags_reflection_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_flags_reflection_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_flags_reflection_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::node_hash_map VARIABLES ############################################

set(abseil_absl_node_hash_map_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_node_hash_map_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_node_hash_map_RES_DIRS_RELEASE )
set(abseil_absl_node_hash_map_DEFINITIONS_RELEASE )
set(abseil_absl_node_hash_map_OBJECTS_RELEASE )
set(abseil_absl_node_hash_map_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_node_hash_map_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_node_hash_map_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_node_hash_map_LIBS_RELEASE )
set(abseil_absl_node_hash_map_SYSTEM_LIBS_RELEASE )
set(abseil_absl_node_hash_map_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_node_hash_map_FRAMEWORKS_RELEASE )
set(abseil_absl_node_hash_map_DEPENDENCIES_RELEASE absl::container_memory absl::core_headers absl::hash_function_defaults absl::node_slot_policy absl::raw_hash_map absl::algorithm_container absl::memory)
set(abseil_absl_node_hash_map_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_node_hash_map_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_node_hash_map_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_node_hash_map_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_node_hash_map_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_node_hash_map_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_node_hash_map_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_node_hash_map_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_node_hash_map_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::flat_hash_map VARIABLES ############################################

set(abseil_absl_flat_hash_map_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_flat_hash_map_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_flat_hash_map_RES_DIRS_RELEASE )
set(abseil_absl_flat_hash_map_DEFINITIONS_RELEASE )
set(abseil_absl_flat_hash_map_OBJECTS_RELEASE )
set(abseil_absl_flat_hash_map_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_flat_hash_map_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_flat_hash_map_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_flat_hash_map_LIBS_RELEASE )
set(abseil_absl_flat_hash_map_SYSTEM_LIBS_RELEASE )
set(abseil_absl_flat_hash_map_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_flat_hash_map_FRAMEWORKS_RELEASE )
set(abseil_absl_flat_hash_map_DEPENDENCIES_RELEASE absl::container_memory absl::core_headers absl::hash_function_defaults absl::raw_hash_map absl::algorithm_container absl::memory)
set(abseil_absl_flat_hash_map_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_flat_hash_map_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_flat_hash_map_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_flat_hash_map_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_flat_hash_map_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_flat_hash_map_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_flat_hash_map_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_flat_hash_map_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_flat_hash_map_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::raw_hash_map VARIABLES ############################################

set(abseil_absl_raw_hash_map_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_raw_hash_map_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_raw_hash_map_RES_DIRS_RELEASE )
set(abseil_absl_raw_hash_map_DEFINITIONS_RELEASE )
set(abseil_absl_raw_hash_map_OBJECTS_RELEASE )
set(abseil_absl_raw_hash_map_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_raw_hash_map_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_raw_hash_map_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_raw_hash_map_LIBS_RELEASE )
set(abseil_absl_raw_hash_map_SYSTEM_LIBS_RELEASE )
set(abseil_absl_raw_hash_map_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_raw_hash_map_FRAMEWORKS_RELEASE )
set(abseil_absl_raw_hash_map_DEPENDENCIES_RELEASE absl::container_memory absl::raw_hash_set absl::throw_delegate)
set(abseil_absl_raw_hash_map_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_raw_hash_map_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_raw_hash_map_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_raw_hash_map_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_raw_hash_map_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_raw_hash_map_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_raw_hash_map_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_raw_hash_map_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_raw_hash_map_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::node_hash_set VARIABLES ############################################

set(abseil_absl_node_hash_set_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_node_hash_set_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_node_hash_set_RES_DIRS_RELEASE )
set(abseil_absl_node_hash_set_DEFINITIONS_RELEASE )
set(abseil_absl_node_hash_set_OBJECTS_RELEASE )
set(abseil_absl_node_hash_set_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_node_hash_set_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_node_hash_set_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_node_hash_set_LIBS_RELEASE )
set(abseil_absl_node_hash_set_SYSTEM_LIBS_RELEASE )
set(abseil_absl_node_hash_set_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_node_hash_set_FRAMEWORKS_RELEASE )
set(abseil_absl_node_hash_set_DEPENDENCIES_RELEASE absl::core_headers absl::hash_function_defaults absl::node_slot_policy absl::raw_hash_set absl::algorithm_container absl::memory)
set(abseil_absl_node_hash_set_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_node_hash_set_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_node_hash_set_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_node_hash_set_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_node_hash_set_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_node_hash_set_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_node_hash_set_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_node_hash_set_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_node_hash_set_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::flat_hash_set VARIABLES ############################################

set(abseil_absl_flat_hash_set_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_flat_hash_set_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_flat_hash_set_RES_DIRS_RELEASE )
set(abseil_absl_flat_hash_set_DEFINITIONS_RELEASE )
set(abseil_absl_flat_hash_set_OBJECTS_RELEASE )
set(abseil_absl_flat_hash_set_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_flat_hash_set_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_flat_hash_set_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_flat_hash_set_LIBS_RELEASE )
set(abseil_absl_flat_hash_set_SYSTEM_LIBS_RELEASE )
set(abseil_absl_flat_hash_set_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_flat_hash_set_FRAMEWORKS_RELEASE )
set(abseil_absl_flat_hash_set_DEPENDENCIES_RELEASE absl::container_memory absl::hash_function_defaults absl::raw_hash_set absl::algorithm_container absl::core_headers absl::memory)
set(abseil_absl_flat_hash_set_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_flat_hash_set_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_flat_hash_set_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_flat_hash_set_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_flat_hash_set_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_flat_hash_set_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_flat_hash_set_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_flat_hash_set_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_flat_hash_set_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::statusor VARIABLES ############################################

set(abseil_absl_statusor_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_statusor_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_statusor_RES_DIRS_RELEASE )
set(abseil_absl_statusor_DEFINITIONS_RELEASE )
set(abseil_absl_statusor_OBJECTS_RELEASE )
set(abseil_absl_statusor_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_statusor_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_statusor_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_statusor_LIBS_RELEASE absl_statusor)
set(abseil_absl_statusor_SYSTEM_LIBS_RELEASE )
set(abseil_absl_statusor_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_statusor_FRAMEWORKS_RELEASE )
set(abseil_absl_statusor_DEPENDENCIES_RELEASE absl::base absl::status absl::core_headers absl::raw_logging_internal absl::type_traits absl::strings absl::utility absl::variant)
set(abseil_absl_statusor_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_statusor_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_statusor_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_statusor_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_statusor_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_statusor_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_statusor_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_statusor_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_statusor_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::status VARIABLES ############################################

set(abseil_absl_status_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_status_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_status_RES_DIRS_RELEASE )
set(abseil_absl_status_DEFINITIONS_RELEASE )
set(abseil_absl_status_OBJECTS_RELEASE )
set(abseil_absl_status_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_status_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_status_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_status_LIBS_RELEASE absl_status)
set(abseil_absl_status_SYSTEM_LIBS_RELEASE )
set(abseil_absl_status_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_status_FRAMEWORKS_RELEASE )
set(abseil_absl_status_DEPENDENCIES_RELEASE absl::atomic_hook absl::config absl::cord absl::core_headers absl::function_ref absl::inlined_vector absl::optional absl::raw_logging_internal absl::stacktrace absl::str_format absl::strerror absl::strings absl::symbolize)
set(abseil_absl_status_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_status_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_status_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_status_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_status_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_status_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_status_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_status_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_status_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::random_random VARIABLES ############################################

set(abseil_absl_random_random_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_random_random_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_random_random_RES_DIRS_RELEASE )
set(abseil_absl_random_random_DEFINITIONS_RELEASE )
set(abseil_absl_random_random_OBJECTS_RELEASE )
set(abseil_absl_random_random_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_random_random_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_random_random_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_random_random_LIBS_RELEASE )
set(abseil_absl_random_random_SYSTEM_LIBS_RELEASE )
set(abseil_absl_random_random_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_random_random_FRAMEWORKS_RELEASE )
set(abseil_absl_random_random_DEPENDENCIES_RELEASE absl::random_distributions absl::random_internal_nonsecure_base absl::random_internal_pcg_engine absl::random_internal_pool_urbg absl::random_internal_randen_engine absl::random_seed_sequences)
set(abseil_absl_random_random_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_random_random_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_random_random_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_random_random_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_random_random_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_random_random_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_random_random_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_random_random_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_random_random_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::raw_hash_set VARIABLES ############################################

set(abseil_absl_raw_hash_set_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_raw_hash_set_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_raw_hash_set_RES_DIRS_RELEASE )
set(abseil_absl_raw_hash_set_DEFINITIONS_RELEASE )
set(abseil_absl_raw_hash_set_OBJECTS_RELEASE )
set(abseil_absl_raw_hash_set_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_raw_hash_set_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_raw_hash_set_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_raw_hash_set_LIBS_RELEASE absl_raw_hash_set)
set(abseil_absl_raw_hash_set_SYSTEM_LIBS_RELEASE )
set(abseil_absl_raw_hash_set_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_raw_hash_set_FRAMEWORKS_RELEASE )
set(abseil_absl_raw_hash_set_DEPENDENCIES_RELEASE absl::bits absl::compressed_tuple absl::config absl::container_common absl::container_memory absl::core_headers absl::endian absl::hash_policy_traits absl::hashtable_debug_hooks absl::memory absl::meta absl::optional absl::prefetch absl::utility absl::hashtablez_sampler)
set(abseil_absl_raw_hash_set_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_raw_hash_set_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_raw_hash_set_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_raw_hash_set_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_raw_hash_set_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_raw_hash_set_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_raw_hash_set_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_raw_hash_set_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_raw_hash_set_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::hashtablez_sampler VARIABLES ############################################

set(abseil_absl_hashtablez_sampler_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_hashtablez_sampler_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_hashtablez_sampler_RES_DIRS_RELEASE )
set(abseil_absl_hashtablez_sampler_DEFINITIONS_RELEASE )
set(abseil_absl_hashtablez_sampler_OBJECTS_RELEASE )
set(abseil_absl_hashtablez_sampler_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_hashtablez_sampler_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_hashtablez_sampler_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_hashtablez_sampler_LIBS_RELEASE absl_hashtablez_sampler)
set(abseil_absl_hashtablez_sampler_SYSTEM_LIBS_RELEASE )
set(abseil_absl_hashtablez_sampler_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_hashtablez_sampler_FRAMEWORKS_RELEASE )
set(abseil_absl_hashtablez_sampler_DEPENDENCIES_RELEASE absl::base absl::config absl::exponential_biased absl::sample_recorder absl::synchronization)
set(abseil_absl_hashtablez_sampler_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_hashtablez_sampler_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_hashtablez_sampler_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_hashtablez_sampler_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_hashtablez_sampler_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_hashtablez_sampler_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_hashtablez_sampler_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_hashtablez_sampler_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_hashtablez_sampler_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::hash_function_defaults VARIABLES ############################################

set(abseil_absl_hash_function_defaults_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_hash_function_defaults_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_hash_function_defaults_RES_DIRS_RELEASE )
set(abseil_absl_hash_function_defaults_DEFINITIONS_RELEASE )
set(abseil_absl_hash_function_defaults_OBJECTS_RELEASE )
set(abseil_absl_hash_function_defaults_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_hash_function_defaults_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_hash_function_defaults_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_hash_function_defaults_LIBS_RELEASE )
set(abseil_absl_hash_function_defaults_SYSTEM_LIBS_RELEASE )
set(abseil_absl_hash_function_defaults_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_hash_function_defaults_FRAMEWORKS_RELEASE )
set(abseil_absl_hash_function_defaults_DEPENDENCIES_RELEASE absl::config absl::cord absl::hash absl::strings)
set(abseil_absl_hash_function_defaults_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_hash_function_defaults_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_hash_function_defaults_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_hash_function_defaults_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_hash_function_defaults_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_hash_function_defaults_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_hash_function_defaults_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_hash_function_defaults_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_hash_function_defaults_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::btree VARIABLES ############################################

set(abseil_absl_btree_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_btree_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_btree_RES_DIRS_RELEASE )
set(abseil_absl_btree_DEFINITIONS_RELEASE )
set(abseil_absl_btree_OBJECTS_RELEASE )
set(abseil_absl_btree_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_btree_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_btree_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_btree_LIBS_RELEASE )
set(abseil_absl_btree_SYSTEM_LIBS_RELEASE )
set(abseil_absl_btree_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_btree_FRAMEWORKS_RELEASE )
set(abseil_absl_btree_DEPENDENCIES_RELEASE absl::container_common absl::compare absl::compressed_tuple absl::container_memory absl::cord absl::core_headers absl::layout absl::memory absl::raw_logging_internal absl::strings absl::throw_delegate absl::type_traits absl::utility)
set(abseil_absl_btree_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_btree_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_btree_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_btree_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_btree_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_btree_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_btree_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_btree_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_btree_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::cord VARIABLES ############################################

set(abseil_absl_cord_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_cord_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_cord_RES_DIRS_RELEASE )
set(abseil_absl_cord_DEFINITIONS_RELEASE )
set(abseil_absl_cord_OBJECTS_RELEASE )
set(abseil_absl_cord_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_cord_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_cord_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_cord_LIBS_RELEASE absl_cord)
set(abseil_absl_cord_SYSTEM_LIBS_RELEASE )
set(abseil_absl_cord_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_cord_FRAMEWORKS_RELEASE )
set(abseil_absl_cord_DEPENDENCIES_RELEASE absl::base absl::config absl::cord_internal absl::cordz_functions absl::cordz_info absl::cordz_update_scope absl::cordz_update_tracker absl::core_headers absl::endian absl::fixed_array absl::function_ref absl::inlined_vector absl::optional absl::raw_logging_internal absl::span absl::strings absl::type_traits)
set(abseil_absl_cord_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_cord_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_cord_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_cord_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_cord_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_cord_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_cord_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_cord_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_cord_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::cordz_update_scope VARIABLES ############################################

set(abseil_absl_cordz_update_scope_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_cordz_update_scope_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_cordz_update_scope_RES_DIRS_RELEASE )
set(abseil_absl_cordz_update_scope_DEFINITIONS_RELEASE )
set(abseil_absl_cordz_update_scope_OBJECTS_RELEASE )
set(abseil_absl_cordz_update_scope_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_cordz_update_scope_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_cordz_update_scope_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_cordz_update_scope_LIBS_RELEASE )
set(abseil_absl_cordz_update_scope_SYSTEM_LIBS_RELEASE )
set(abseil_absl_cordz_update_scope_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_cordz_update_scope_FRAMEWORKS_RELEASE )
set(abseil_absl_cordz_update_scope_DEPENDENCIES_RELEASE absl::config absl::cord_internal absl::cordz_info absl::cordz_update_tracker absl::core_headers)
set(abseil_absl_cordz_update_scope_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_cordz_update_scope_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_cordz_update_scope_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_cordz_update_scope_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_cordz_update_scope_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_cordz_update_scope_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_cordz_update_scope_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_cordz_update_scope_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_cordz_update_scope_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::cordz_sample_token VARIABLES ############################################

set(abseil_absl_cordz_sample_token_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_cordz_sample_token_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_cordz_sample_token_RES_DIRS_RELEASE )
set(abseil_absl_cordz_sample_token_DEFINITIONS_RELEASE )
set(abseil_absl_cordz_sample_token_OBJECTS_RELEASE )
set(abseil_absl_cordz_sample_token_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_cordz_sample_token_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_cordz_sample_token_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_cordz_sample_token_LIBS_RELEASE absl_cordz_sample_token)
set(abseil_absl_cordz_sample_token_SYSTEM_LIBS_RELEASE )
set(abseil_absl_cordz_sample_token_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_cordz_sample_token_FRAMEWORKS_RELEASE )
set(abseil_absl_cordz_sample_token_DEPENDENCIES_RELEASE absl::config absl::cordz_handle absl::cordz_info)
set(abseil_absl_cordz_sample_token_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_cordz_sample_token_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_cordz_sample_token_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_cordz_sample_token_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_cordz_sample_token_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_cordz_sample_token_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_cordz_sample_token_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_cordz_sample_token_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_cordz_sample_token_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::cordz_info VARIABLES ############################################

set(abseil_absl_cordz_info_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_cordz_info_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_cordz_info_RES_DIRS_RELEASE )
set(abseil_absl_cordz_info_DEFINITIONS_RELEASE )
set(abseil_absl_cordz_info_OBJECTS_RELEASE )
set(abseil_absl_cordz_info_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_cordz_info_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_cordz_info_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_cordz_info_LIBS_RELEASE absl_cordz_info)
set(abseil_absl_cordz_info_SYSTEM_LIBS_RELEASE )
set(abseil_absl_cordz_info_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_cordz_info_FRAMEWORKS_RELEASE )
set(abseil_absl_cordz_info_DEPENDENCIES_RELEASE absl::base absl::config absl::cord_internal absl::cordz_functions absl::cordz_handle absl::cordz_statistics absl::cordz_update_tracker absl::core_headers absl::inlined_vector absl::span absl::raw_logging_internal absl::stacktrace absl::synchronization)
set(abseil_absl_cordz_info_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_cordz_info_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_cordz_info_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_cordz_info_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_cordz_info_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_cordz_info_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_cordz_info_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_cordz_info_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_cordz_info_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::cordz_handle VARIABLES ############################################

set(abseil_absl_cordz_handle_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_cordz_handle_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_cordz_handle_RES_DIRS_RELEASE )
set(abseil_absl_cordz_handle_DEFINITIONS_RELEASE )
set(abseil_absl_cordz_handle_OBJECTS_RELEASE )
set(abseil_absl_cordz_handle_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_cordz_handle_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_cordz_handle_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_cordz_handle_LIBS_RELEASE absl_cordz_handle)
set(abseil_absl_cordz_handle_SYSTEM_LIBS_RELEASE )
set(abseil_absl_cordz_handle_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_cordz_handle_FRAMEWORKS_RELEASE )
set(abseil_absl_cordz_handle_DEPENDENCIES_RELEASE absl::base absl::config absl::raw_logging_internal absl::synchronization)
set(abseil_absl_cordz_handle_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_cordz_handle_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_cordz_handle_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_cordz_handle_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_cordz_handle_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_cordz_handle_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_cordz_handle_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_cordz_handle_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_cordz_handle_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::cordz_statistics VARIABLES ############################################

set(abseil_absl_cordz_statistics_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_cordz_statistics_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_cordz_statistics_RES_DIRS_RELEASE )
set(abseil_absl_cordz_statistics_DEFINITIONS_RELEASE )
set(abseil_absl_cordz_statistics_OBJECTS_RELEASE )
set(abseil_absl_cordz_statistics_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_cordz_statistics_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_cordz_statistics_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_cordz_statistics_LIBS_RELEASE )
set(abseil_absl_cordz_statistics_SYSTEM_LIBS_RELEASE )
set(abseil_absl_cordz_statistics_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_cordz_statistics_FRAMEWORKS_RELEASE )
set(abseil_absl_cordz_statistics_DEPENDENCIES_RELEASE absl::config absl::core_headers absl::cordz_update_tracker absl::synchronization)
set(abseil_absl_cordz_statistics_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_cordz_statistics_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_cordz_statistics_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_cordz_statistics_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_cordz_statistics_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_cordz_statistics_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_cordz_statistics_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_cordz_statistics_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_cordz_statistics_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::random_internal_distribution_test_util VARIABLES ############################################

set(abseil_absl_random_internal_distribution_test_util_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_random_internal_distribution_test_util_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_random_internal_distribution_test_util_RES_DIRS_RELEASE )
set(abseil_absl_random_internal_distribution_test_util_DEFINITIONS_RELEASE )
set(abseil_absl_random_internal_distribution_test_util_OBJECTS_RELEASE )
set(abseil_absl_random_internal_distribution_test_util_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_random_internal_distribution_test_util_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_random_internal_distribution_test_util_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_random_internal_distribution_test_util_LIBS_RELEASE absl_random_internal_distribution_test_util)
set(abseil_absl_random_internal_distribution_test_util_SYSTEM_LIBS_RELEASE )
set(abseil_absl_random_internal_distribution_test_util_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_random_internal_distribution_test_util_FRAMEWORKS_RELEASE )
set(abseil_absl_random_internal_distribution_test_util_DEPENDENCIES_RELEASE absl::config absl::core_headers absl::raw_logging_internal absl::strings absl::str_format absl::span)
set(abseil_absl_random_internal_distribution_test_util_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_random_internal_distribution_test_util_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_random_internal_distribution_test_util_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_random_internal_distribution_test_util_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_random_internal_distribution_test_util_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_random_internal_distribution_test_util_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_random_internal_distribution_test_util_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_random_internal_distribution_test_util_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_random_internal_distribution_test_util_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::random_seed_sequences VARIABLES ############################################

set(abseil_absl_random_seed_sequences_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_random_seed_sequences_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_random_seed_sequences_RES_DIRS_RELEASE )
set(abseil_absl_random_seed_sequences_DEFINITIONS_RELEASE )
set(abseil_absl_random_seed_sequences_OBJECTS_RELEASE )
set(abseil_absl_random_seed_sequences_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_random_seed_sequences_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_random_seed_sequences_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_random_seed_sequences_LIBS_RELEASE absl_random_seed_sequences)
set(abseil_absl_random_seed_sequences_SYSTEM_LIBS_RELEASE )
set(abseil_absl_random_seed_sequences_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_random_seed_sequences_FRAMEWORKS_RELEASE )
set(abseil_absl_random_seed_sequences_DEPENDENCIES_RELEASE absl::config absl::inlined_vector absl::random_internal_pool_urbg absl::random_internal_salted_seed_seq absl::random_internal_seed_material absl::random_seed_gen_exception absl::span)
set(abseil_absl_random_seed_sequences_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_random_seed_sequences_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_random_seed_sequences_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_random_seed_sequences_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_random_seed_sequences_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_random_seed_sequences_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_random_seed_sequences_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_random_seed_sequences_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_random_seed_sequences_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::sample_recorder VARIABLES ############################################

set(abseil_absl_sample_recorder_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_sample_recorder_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_sample_recorder_RES_DIRS_RELEASE )
set(abseil_absl_sample_recorder_DEFINITIONS_RELEASE )
set(abseil_absl_sample_recorder_OBJECTS_RELEASE )
set(abseil_absl_sample_recorder_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_sample_recorder_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_sample_recorder_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_sample_recorder_LIBS_RELEASE )
set(abseil_absl_sample_recorder_SYSTEM_LIBS_RELEASE )
set(abseil_absl_sample_recorder_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_sample_recorder_FRAMEWORKS_RELEASE )
set(abseil_absl_sample_recorder_DEPENDENCIES_RELEASE absl::base absl::synchronization)
set(abseil_absl_sample_recorder_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_sample_recorder_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_sample_recorder_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_sample_recorder_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_sample_recorder_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_sample_recorder_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_sample_recorder_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_sample_recorder_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_sample_recorder_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::flags_internal VARIABLES ############################################

set(abseil_absl_flags_internal_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_flags_internal_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_flags_internal_RES_DIRS_RELEASE )
set(abseil_absl_flags_internal_DEFINITIONS_RELEASE )
set(abseil_absl_flags_internal_OBJECTS_RELEASE )
set(abseil_absl_flags_internal_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_flags_internal_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_flags_internal_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_flags_internal_LIBS_RELEASE absl_flags_internal)
set(abseil_absl_flags_internal_SYSTEM_LIBS_RELEASE )
set(abseil_absl_flags_internal_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_flags_internal_FRAMEWORKS_RELEASE )
set(abseil_absl_flags_internal_DEPENDENCIES_RELEASE absl::base absl::config absl::flags_commandlineflag absl::flags_commandlineflag_internal absl::flags_config absl::flags_marshalling absl::synchronization absl::meta absl::utility)
set(abseil_absl_flags_internal_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_flags_internal_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_flags_internal_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_flags_internal_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_flags_internal_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_flags_internal_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_flags_internal_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_flags_internal_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_flags_internal_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::flags_marshalling VARIABLES ############################################

set(abseil_absl_flags_marshalling_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_flags_marshalling_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_flags_marshalling_RES_DIRS_RELEASE )
set(abseil_absl_flags_marshalling_DEFINITIONS_RELEASE )
set(abseil_absl_flags_marshalling_OBJECTS_RELEASE )
set(abseil_absl_flags_marshalling_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_flags_marshalling_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_flags_marshalling_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_flags_marshalling_LIBS_RELEASE absl_flags_marshalling)
set(abseil_absl_flags_marshalling_SYSTEM_LIBS_RELEASE )
set(abseil_absl_flags_marshalling_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_flags_marshalling_FRAMEWORKS_RELEASE )
set(abseil_absl_flags_marshalling_DEPENDENCIES_RELEASE absl::config absl::core_headers absl::log_severity absl::optional absl::strings absl::str_format)
set(abseil_absl_flags_marshalling_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_flags_marshalling_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_flags_marshalling_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_flags_marshalling_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_flags_marshalling_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_flags_marshalling_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_flags_marshalling_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_flags_marshalling_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_flags_marshalling_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::flags_config VARIABLES ############################################

set(abseil_absl_flags_config_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_flags_config_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_flags_config_RES_DIRS_RELEASE )
set(abseil_absl_flags_config_DEFINITIONS_RELEASE )
set(abseil_absl_flags_config_OBJECTS_RELEASE )
set(abseil_absl_flags_config_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_flags_config_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_flags_config_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_flags_config_LIBS_RELEASE absl_flags_config)
set(abseil_absl_flags_config_SYSTEM_LIBS_RELEASE )
set(abseil_absl_flags_config_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_flags_config_FRAMEWORKS_RELEASE )
set(abseil_absl_flags_config_DEPENDENCIES_RELEASE absl::config absl::flags_path_util absl::flags_program_name absl::core_headers absl::strings absl::synchronization)
set(abseil_absl_flags_config_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_flags_config_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_flags_config_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_flags_config_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_flags_config_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_flags_config_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_flags_config_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_flags_config_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_flags_config_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::flags_program_name VARIABLES ############################################

set(abseil_absl_flags_program_name_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_flags_program_name_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_flags_program_name_RES_DIRS_RELEASE )
set(abseil_absl_flags_program_name_DEFINITIONS_RELEASE )
set(abseil_absl_flags_program_name_OBJECTS_RELEASE )
set(abseil_absl_flags_program_name_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_flags_program_name_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_flags_program_name_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_flags_program_name_LIBS_RELEASE absl_flags_program_name)
set(abseil_absl_flags_program_name_SYSTEM_LIBS_RELEASE )
set(abseil_absl_flags_program_name_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_flags_program_name_FRAMEWORKS_RELEASE )
set(abseil_absl_flags_program_name_DEPENDENCIES_RELEASE absl::config absl::core_headers absl::flags_path_util absl::strings absl::synchronization)
set(abseil_absl_flags_program_name_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_flags_program_name_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_flags_program_name_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_flags_program_name_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_flags_program_name_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_flags_program_name_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_flags_program_name_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_flags_program_name_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_flags_program_name_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::synchronization VARIABLES ############################################

set(abseil_absl_synchronization_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_synchronization_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_synchronization_RES_DIRS_RELEASE )
set(abseil_absl_synchronization_DEFINITIONS_RELEASE )
set(abseil_absl_synchronization_OBJECTS_RELEASE )
set(abseil_absl_synchronization_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_synchronization_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_synchronization_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_synchronization_LIBS_RELEASE absl_synchronization)
set(abseil_absl_synchronization_SYSTEM_LIBS_RELEASE )
set(abseil_absl_synchronization_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_synchronization_FRAMEWORKS_RELEASE )
set(abseil_absl_synchronization_DEPENDENCIES_RELEASE absl::graphcycles_internal absl::kernel_timeout_internal absl::atomic_hook absl::base absl::base_internal absl::config absl::core_headers absl::dynamic_annotations absl::malloc_internal absl::raw_logging_internal absl::stacktrace absl::symbolize absl::time)
set(abseil_absl_synchronization_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_synchronization_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_synchronization_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_synchronization_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_synchronization_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_synchronization_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_synchronization_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_synchronization_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_synchronization_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::kernel_timeout_internal VARIABLES ############################################

set(abseil_absl_kernel_timeout_internal_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_kernel_timeout_internal_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_kernel_timeout_internal_RES_DIRS_RELEASE )
set(abseil_absl_kernel_timeout_internal_DEFINITIONS_RELEASE )
set(abseil_absl_kernel_timeout_internal_OBJECTS_RELEASE )
set(abseil_absl_kernel_timeout_internal_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_kernel_timeout_internal_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_kernel_timeout_internal_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_kernel_timeout_internal_LIBS_RELEASE )
set(abseil_absl_kernel_timeout_internal_SYSTEM_LIBS_RELEASE )
set(abseil_absl_kernel_timeout_internal_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_kernel_timeout_internal_FRAMEWORKS_RELEASE )
set(abseil_absl_kernel_timeout_internal_DEPENDENCIES_RELEASE absl::core_headers absl::raw_logging_internal absl::time)
set(abseil_absl_kernel_timeout_internal_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_kernel_timeout_internal_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_kernel_timeout_internal_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_kernel_timeout_internal_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_kernel_timeout_internal_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_kernel_timeout_internal_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_kernel_timeout_internal_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_kernel_timeout_internal_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_kernel_timeout_internal_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::cord_internal VARIABLES ############################################

set(abseil_absl_cord_internal_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_cord_internal_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_cord_internal_RES_DIRS_RELEASE )
set(abseil_absl_cord_internal_DEFINITIONS_RELEASE )
set(abseil_absl_cord_internal_OBJECTS_RELEASE )
set(abseil_absl_cord_internal_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_cord_internal_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_cord_internal_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_cord_internal_LIBS_RELEASE absl_cord_internal)
set(abseil_absl_cord_internal_SYSTEM_LIBS_RELEASE )
set(abseil_absl_cord_internal_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_cord_internal_FRAMEWORKS_RELEASE )
set(abseil_absl_cord_internal_DEPENDENCIES_RELEASE absl::base_internal absl::compressed_tuple absl::config absl::core_headers absl::endian absl::inlined_vector absl::layout absl::raw_logging_internal absl::strings absl::throw_delegate absl::type_traits)
set(abseil_absl_cord_internal_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_cord_internal_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_cord_internal_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_cord_internal_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_cord_internal_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_cord_internal_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_cord_internal_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_cord_internal_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_cord_internal_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::str_format VARIABLES ############################################

set(abseil_absl_str_format_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_str_format_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_str_format_RES_DIRS_RELEASE )
set(abseil_absl_str_format_DEFINITIONS_RELEASE )
set(abseil_absl_str_format_OBJECTS_RELEASE )
set(abseil_absl_str_format_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_str_format_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_str_format_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_str_format_LIBS_RELEASE )
set(abseil_absl_str_format_SYSTEM_LIBS_RELEASE )
set(abseil_absl_str_format_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_str_format_FRAMEWORKS_RELEASE )
set(abseil_absl_str_format_DEPENDENCIES_RELEASE absl::str_format_internal)
set(abseil_absl_str_format_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_str_format_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_str_format_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_str_format_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_str_format_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_str_format_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_str_format_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_str_format_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_str_format_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::random_internal_nonsecure_base VARIABLES ############################################

set(abseil_absl_random_internal_nonsecure_base_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_random_internal_nonsecure_base_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_random_internal_nonsecure_base_RES_DIRS_RELEASE )
set(abseil_absl_random_internal_nonsecure_base_DEFINITIONS_RELEASE )
set(abseil_absl_random_internal_nonsecure_base_OBJECTS_RELEASE )
set(abseil_absl_random_internal_nonsecure_base_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_random_internal_nonsecure_base_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_random_internal_nonsecure_base_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_random_internal_nonsecure_base_LIBS_RELEASE )
set(abseil_absl_random_internal_nonsecure_base_SYSTEM_LIBS_RELEASE )
set(abseil_absl_random_internal_nonsecure_base_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_random_internal_nonsecure_base_FRAMEWORKS_RELEASE )
set(abseil_absl_random_internal_nonsecure_base_DEPENDENCIES_RELEASE absl::core_headers absl::inlined_vector absl::random_internal_pool_urbg absl::random_internal_salted_seed_seq absl::random_internal_seed_material absl::span absl::type_traits)
set(abseil_absl_random_internal_nonsecure_base_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_random_internal_nonsecure_base_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_random_internal_nonsecure_base_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_random_internal_nonsecure_base_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_random_internal_nonsecure_base_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_random_internal_nonsecure_base_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_random_internal_nonsecure_base_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_random_internal_nonsecure_base_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_random_internal_nonsecure_base_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::random_internal_salted_seed_seq VARIABLES ############################################

set(abseil_absl_random_internal_salted_seed_seq_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_random_internal_salted_seed_seq_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_random_internal_salted_seed_seq_RES_DIRS_RELEASE )
set(abseil_absl_random_internal_salted_seed_seq_DEFINITIONS_RELEASE )
set(abseil_absl_random_internal_salted_seed_seq_OBJECTS_RELEASE )
set(abseil_absl_random_internal_salted_seed_seq_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_random_internal_salted_seed_seq_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_random_internal_salted_seed_seq_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_random_internal_salted_seed_seq_LIBS_RELEASE )
set(abseil_absl_random_internal_salted_seed_seq_SYSTEM_LIBS_RELEASE )
set(abseil_absl_random_internal_salted_seed_seq_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_random_internal_salted_seed_seq_FRAMEWORKS_RELEASE )
set(abseil_absl_random_internal_salted_seed_seq_DEPENDENCIES_RELEASE absl::inlined_vector absl::optional absl::span absl::random_internal_seed_material absl::type_traits)
set(abseil_absl_random_internal_salted_seed_seq_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_random_internal_salted_seed_seq_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_random_internal_salted_seed_seq_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_random_internal_salted_seed_seq_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_random_internal_salted_seed_seq_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_random_internal_salted_seed_seq_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_random_internal_salted_seed_seq_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_random_internal_salted_seed_seq_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_random_internal_salted_seed_seq_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::random_internal_pool_urbg VARIABLES ############################################

set(abseil_absl_random_internal_pool_urbg_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_random_internal_pool_urbg_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_random_internal_pool_urbg_RES_DIRS_RELEASE )
set(abseil_absl_random_internal_pool_urbg_DEFINITIONS_RELEASE )
set(abseil_absl_random_internal_pool_urbg_OBJECTS_RELEASE )
set(abseil_absl_random_internal_pool_urbg_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_random_internal_pool_urbg_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_random_internal_pool_urbg_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_random_internal_pool_urbg_LIBS_RELEASE absl_random_internal_pool_urbg)
set(abseil_absl_random_internal_pool_urbg_SYSTEM_LIBS_RELEASE )
set(abseil_absl_random_internal_pool_urbg_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_random_internal_pool_urbg_FRAMEWORKS_RELEASE )
set(abseil_absl_random_internal_pool_urbg_DEPENDENCIES_RELEASE absl::base absl::config absl::core_headers absl::endian absl::random_internal_randen absl::random_internal_seed_material absl::random_internal_traits absl::random_seed_gen_exception absl::raw_logging_internal absl::span)
set(abseil_absl_random_internal_pool_urbg_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_random_internal_pool_urbg_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_random_internal_pool_urbg_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_random_internal_pool_urbg_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_random_internal_pool_urbg_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_random_internal_pool_urbg_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_random_internal_pool_urbg_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_random_internal_pool_urbg_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_random_internal_pool_urbg_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::random_internal_seed_material VARIABLES ############################################

set(abseil_absl_random_internal_seed_material_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_random_internal_seed_material_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_random_internal_seed_material_RES_DIRS_RELEASE )
set(abseil_absl_random_internal_seed_material_DEFINITIONS_RELEASE )
set(abseil_absl_random_internal_seed_material_OBJECTS_RELEASE )
set(abseil_absl_random_internal_seed_material_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_random_internal_seed_material_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_random_internal_seed_material_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_random_internal_seed_material_LIBS_RELEASE absl_random_internal_seed_material)
set(abseil_absl_random_internal_seed_material_SYSTEM_LIBS_RELEASE )
set(abseil_absl_random_internal_seed_material_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_random_internal_seed_material_FRAMEWORKS_RELEASE )
set(abseil_absl_random_internal_seed_material_DEPENDENCIES_RELEASE absl::core_headers absl::optional absl::random_internal_fast_uniform_bits absl::raw_logging_internal absl::span absl::strings)
set(abseil_absl_random_internal_seed_material_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_random_internal_seed_material_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_random_internal_seed_material_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_random_internal_seed_material_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_random_internal_seed_material_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_random_internal_seed_material_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_random_internal_seed_material_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_random_internal_seed_material_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_random_internal_seed_material_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::random_distributions VARIABLES ############################################

set(abseil_absl_random_distributions_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_random_distributions_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_random_distributions_RES_DIRS_RELEASE )
set(abseil_absl_random_distributions_DEFINITIONS_RELEASE )
set(abseil_absl_random_distributions_OBJECTS_RELEASE )
set(abseil_absl_random_distributions_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_random_distributions_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_random_distributions_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_random_distributions_LIBS_RELEASE absl_random_distributions)
set(abseil_absl_random_distributions_SYSTEM_LIBS_RELEASE )
set(abseil_absl_random_distributions_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_random_distributions_FRAMEWORKS_RELEASE )
set(abseil_absl_random_distributions_DEPENDENCIES_RELEASE absl::base_internal absl::config absl::core_headers absl::random_internal_generate_real absl::random_internal_distribution_caller absl::random_internal_fast_uniform_bits absl::random_internal_fastmath absl::random_internal_iostream_state_saver absl::random_internal_traits absl::random_internal_uniform_helper absl::random_internal_wide_multiply absl::strings absl::type_traits)
set(abseil_absl_random_distributions_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_random_distributions_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_random_distributions_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_random_distributions_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_random_distributions_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_random_distributions_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_random_distributions_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_random_distributions_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_random_distributions_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::hash VARIABLES ############################################

set(abseil_absl_hash_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_hash_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_hash_RES_DIRS_RELEASE )
set(abseil_absl_hash_DEFINITIONS_RELEASE )
set(abseil_absl_hash_OBJECTS_RELEASE )
set(abseil_absl_hash_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_hash_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_hash_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_hash_LIBS_RELEASE absl_hash)
set(abseil_absl_hash_SYSTEM_LIBS_RELEASE )
set(abseil_absl_hash_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_hash_FRAMEWORKS_RELEASE )
set(abseil_absl_hash_DEPENDENCIES_RELEASE absl::city absl::config absl::core_headers absl::endian absl::fixed_array absl::function_ref absl::meta absl::int128 absl::strings absl::optional absl::variant absl::utility absl::low_level_hash)
set(abseil_absl_hash_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_hash_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_hash_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_hash_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_hash_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_hash_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_hash_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_hash_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_hash_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::flags_private_handle_accessor VARIABLES ############################################

set(abseil_absl_flags_private_handle_accessor_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_flags_private_handle_accessor_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_flags_private_handle_accessor_RES_DIRS_RELEASE )
set(abseil_absl_flags_private_handle_accessor_DEFINITIONS_RELEASE )
set(abseil_absl_flags_private_handle_accessor_OBJECTS_RELEASE )
set(abseil_absl_flags_private_handle_accessor_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_flags_private_handle_accessor_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_flags_private_handle_accessor_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_flags_private_handle_accessor_LIBS_RELEASE absl_flags_private_handle_accessor)
set(abseil_absl_flags_private_handle_accessor_SYSTEM_LIBS_RELEASE )
set(abseil_absl_flags_private_handle_accessor_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_flags_private_handle_accessor_FRAMEWORKS_RELEASE )
set(abseil_absl_flags_private_handle_accessor_DEPENDENCIES_RELEASE absl::config absl::flags_commandlineflag absl::flags_commandlineflag_internal absl::strings)
set(abseil_absl_flags_private_handle_accessor_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_flags_private_handle_accessor_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_flags_private_handle_accessor_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_flags_private_handle_accessor_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_flags_private_handle_accessor_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_flags_private_handle_accessor_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_flags_private_handle_accessor_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_flags_private_handle_accessor_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_flags_private_handle_accessor_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::flags_commandlineflag VARIABLES ############################################

set(abseil_absl_flags_commandlineflag_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_flags_commandlineflag_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_flags_commandlineflag_RES_DIRS_RELEASE )
set(abseil_absl_flags_commandlineflag_DEFINITIONS_RELEASE )
set(abseil_absl_flags_commandlineflag_OBJECTS_RELEASE )
set(abseil_absl_flags_commandlineflag_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_flags_commandlineflag_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_flags_commandlineflag_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_flags_commandlineflag_LIBS_RELEASE absl_flags_commandlineflag)
set(abseil_absl_flags_commandlineflag_SYSTEM_LIBS_RELEASE )
set(abseil_absl_flags_commandlineflag_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_flags_commandlineflag_FRAMEWORKS_RELEASE )
set(abseil_absl_flags_commandlineflag_DEPENDENCIES_RELEASE absl::config absl::fast_type_id absl::flags_commandlineflag_internal absl::optional absl::strings)
set(abseil_absl_flags_commandlineflag_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_flags_commandlineflag_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_flags_commandlineflag_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_flags_commandlineflag_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_flags_commandlineflag_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_flags_commandlineflag_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_flags_commandlineflag_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_flags_commandlineflag_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_flags_commandlineflag_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::flags_path_util VARIABLES ############################################

set(abseil_absl_flags_path_util_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_flags_path_util_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_flags_path_util_RES_DIRS_RELEASE )
set(abseil_absl_flags_path_util_DEFINITIONS_RELEASE )
set(abseil_absl_flags_path_util_OBJECTS_RELEASE )
set(abseil_absl_flags_path_util_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_flags_path_util_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_flags_path_util_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_flags_path_util_LIBS_RELEASE )
set(abseil_absl_flags_path_util_SYSTEM_LIBS_RELEASE )
set(abseil_absl_flags_path_util_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_flags_path_util_FRAMEWORKS_RELEASE )
set(abseil_absl_flags_path_util_DEPENDENCIES_RELEASE absl::config absl::strings)
set(abseil_absl_flags_path_util_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_flags_path_util_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_flags_path_util_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_flags_path_util_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_flags_path_util_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_flags_path_util_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_flags_path_util_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_flags_path_util_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_flags_path_util_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::failure_signal_handler VARIABLES ############################################

set(abseil_absl_failure_signal_handler_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_failure_signal_handler_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_failure_signal_handler_RES_DIRS_RELEASE )
set(abseil_absl_failure_signal_handler_DEFINITIONS_RELEASE )
set(abseil_absl_failure_signal_handler_OBJECTS_RELEASE )
set(abseil_absl_failure_signal_handler_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_failure_signal_handler_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_failure_signal_handler_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_failure_signal_handler_LIBS_RELEASE absl_failure_signal_handler)
set(abseil_absl_failure_signal_handler_SYSTEM_LIBS_RELEASE )
set(abseil_absl_failure_signal_handler_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_failure_signal_handler_FRAMEWORKS_RELEASE )
set(abseil_absl_failure_signal_handler_DEPENDENCIES_RELEASE absl::examine_stack absl::stacktrace absl::base absl::config absl::core_headers absl::raw_logging_internal)
set(abseil_absl_failure_signal_handler_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_failure_signal_handler_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_failure_signal_handler_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_failure_signal_handler_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_failure_signal_handler_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_failure_signal_handler_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_failure_signal_handler_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_failure_signal_handler_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_failure_signal_handler_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::examine_stack VARIABLES ############################################

set(abseil_absl_examine_stack_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_examine_stack_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_examine_stack_RES_DIRS_RELEASE )
set(abseil_absl_examine_stack_DEFINITIONS_RELEASE )
set(abseil_absl_examine_stack_OBJECTS_RELEASE )
set(abseil_absl_examine_stack_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_examine_stack_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_examine_stack_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_examine_stack_LIBS_RELEASE absl_examine_stack)
set(abseil_absl_examine_stack_SYSTEM_LIBS_RELEASE )
set(abseil_absl_examine_stack_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_examine_stack_FRAMEWORKS_RELEASE )
set(abseil_absl_examine_stack_DEPENDENCIES_RELEASE absl::stacktrace absl::symbolize absl::config absl::core_headers absl::raw_logging_internal)
set(abseil_absl_examine_stack_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_examine_stack_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_examine_stack_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_examine_stack_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_examine_stack_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_examine_stack_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_examine_stack_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_examine_stack_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_examine_stack_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::symbolize VARIABLES ############################################

set(abseil_absl_symbolize_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_symbolize_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_symbolize_RES_DIRS_RELEASE )
set(abseil_absl_symbolize_DEFINITIONS_RELEASE )
set(abseil_absl_symbolize_OBJECTS_RELEASE )
set(abseil_absl_symbolize_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_symbolize_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_symbolize_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_symbolize_LIBS_RELEASE absl_symbolize)
set(abseil_absl_symbolize_SYSTEM_LIBS_RELEASE )
set(abseil_absl_symbolize_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_symbolize_FRAMEWORKS_RELEASE )
set(abseil_absl_symbolize_DEPENDENCIES_RELEASE absl::debugging_internal absl::demangle_internal absl::base absl::config absl::core_headers absl::dynamic_annotations absl::malloc_internal absl::raw_logging_internal absl::strings)
set(abseil_absl_symbolize_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_symbolize_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_symbolize_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_symbolize_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_symbolize_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_symbolize_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_symbolize_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_symbolize_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_symbolize_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::layout VARIABLES ############################################

set(abseil_absl_layout_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_layout_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_layout_RES_DIRS_RELEASE )
set(abseil_absl_layout_DEFINITIONS_RELEASE )
set(abseil_absl_layout_OBJECTS_RELEASE )
set(abseil_absl_layout_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_layout_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_layout_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_layout_LIBS_RELEASE )
set(abseil_absl_layout_SYSTEM_LIBS_RELEASE )
set(abseil_absl_layout_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_layout_FRAMEWORKS_RELEASE )
set(abseil_absl_layout_DEPENDENCIES_RELEASE absl::config absl::core_headers absl::meta absl::strings absl::span absl::utility)
set(abseil_absl_layout_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_layout_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_layout_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_layout_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_layout_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_layout_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_layout_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_layout_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_layout_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::any VARIABLES ############################################

set(abseil_absl_any_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_any_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_any_RES_DIRS_RELEASE )
set(abseil_absl_any_DEFINITIONS_RELEASE )
set(abseil_absl_any_OBJECTS_RELEASE )
set(abseil_absl_any_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_any_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_any_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_any_LIBS_RELEASE )
set(abseil_absl_any_SYSTEM_LIBS_RELEASE )
set(abseil_absl_any_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_any_FRAMEWORKS_RELEASE )
set(abseil_absl_any_DEPENDENCIES_RELEASE absl::bad_any_cast absl::config absl::core_headers absl::fast_type_id absl::type_traits absl::utility)
set(abseil_absl_any_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_any_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_any_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_any_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_any_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_any_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_any_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_any_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_any_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::time VARIABLES ############################################

set(abseil_absl_time_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_time_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_time_RES_DIRS_RELEASE )
set(abseil_absl_time_DEFINITIONS_RELEASE )
set(abseil_absl_time_OBJECTS_RELEASE )
set(abseil_absl_time_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_time_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_time_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_time_LIBS_RELEASE absl_time)
set(abseil_absl_time_SYSTEM_LIBS_RELEASE )
set(abseil_absl_time_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_time_FRAMEWORKS_RELEASE )
set(abseil_absl_time_DEPENDENCIES_RELEASE absl::base absl::civil_time absl::core_headers absl::int128 absl::raw_logging_internal absl::strings absl::time_zone)
set(abseil_absl_time_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_time_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_time_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_time_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_time_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_time_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_time_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_time_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_time_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::graphcycles_internal VARIABLES ############################################

set(abseil_absl_graphcycles_internal_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_graphcycles_internal_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_graphcycles_internal_RES_DIRS_RELEASE )
set(abseil_absl_graphcycles_internal_DEFINITIONS_RELEASE )
set(abseil_absl_graphcycles_internal_OBJECTS_RELEASE )
set(abseil_absl_graphcycles_internal_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_graphcycles_internal_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_graphcycles_internal_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_graphcycles_internal_LIBS_RELEASE absl_graphcycles_internal)
set(abseil_absl_graphcycles_internal_SYSTEM_LIBS_RELEASE )
set(abseil_absl_graphcycles_internal_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_graphcycles_internal_FRAMEWORKS_RELEASE )
set(abseil_absl_graphcycles_internal_DEPENDENCIES_RELEASE absl::base absl::base_internal absl::config absl::core_headers absl::malloc_internal absl::raw_logging_internal)
set(abseil_absl_graphcycles_internal_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_graphcycles_internal_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_graphcycles_internal_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_graphcycles_internal_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_graphcycles_internal_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_graphcycles_internal_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_graphcycles_internal_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_graphcycles_internal_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_graphcycles_internal_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::str_format_internal VARIABLES ############################################

set(abseil_absl_str_format_internal_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_str_format_internal_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_str_format_internal_RES_DIRS_RELEASE )
set(abseil_absl_str_format_internal_DEFINITIONS_RELEASE )
set(abseil_absl_str_format_internal_OBJECTS_RELEASE )
set(abseil_absl_str_format_internal_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_str_format_internal_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_str_format_internal_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_str_format_internal_LIBS_RELEASE absl_str_format_internal)
set(abseil_absl_str_format_internal_SYSTEM_LIBS_RELEASE )
set(abseil_absl_str_format_internal_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_str_format_internal_FRAMEWORKS_RELEASE )
set(abseil_absl_str_format_internal_DEPENDENCIES_RELEASE absl::bits absl::strings absl::config absl::core_headers absl::numeric_representation absl::type_traits absl::utility absl::int128 absl::span)
set(abseil_absl_str_format_internal_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_str_format_internal_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_str_format_internal_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_str_format_internal_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_str_format_internal_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_str_format_internal_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_str_format_internal_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_str_format_internal_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_str_format_internal_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::strings VARIABLES ############################################

set(abseil_absl_strings_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_strings_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_strings_RES_DIRS_RELEASE )
set(abseil_absl_strings_DEFINITIONS_RELEASE )
set(abseil_absl_strings_OBJECTS_RELEASE )
set(abseil_absl_strings_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_strings_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_strings_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_strings_LIBS_RELEASE absl_strings)
set(abseil_absl_strings_SYSTEM_LIBS_RELEASE )
set(abseil_absl_strings_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_strings_FRAMEWORKS_RELEASE )
set(abseil_absl_strings_DEPENDENCIES_RELEASE absl::strings_internal absl::base absl::bits absl::config absl::core_headers absl::endian absl::int128 absl::memory absl::raw_logging_internal absl::throw_delegate absl::type_traits)
set(abseil_absl_strings_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_strings_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_strings_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_strings_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_strings_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_strings_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_strings_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_strings_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_strings_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::random_internal_randen_engine VARIABLES ############################################

set(abseil_absl_random_internal_randen_engine_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_random_internal_randen_engine_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_random_internal_randen_engine_RES_DIRS_RELEASE )
set(abseil_absl_random_internal_randen_engine_DEFINITIONS_RELEASE )
set(abseil_absl_random_internal_randen_engine_OBJECTS_RELEASE )
set(abseil_absl_random_internal_randen_engine_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_random_internal_randen_engine_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_random_internal_randen_engine_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_random_internal_randen_engine_LIBS_RELEASE )
set(abseil_absl_random_internal_randen_engine_SYSTEM_LIBS_RELEASE )
set(abseil_absl_random_internal_randen_engine_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_random_internal_randen_engine_FRAMEWORKS_RELEASE )
set(abseil_absl_random_internal_randen_engine_DEPENDENCIES_RELEASE absl::endian absl::random_internal_iostream_state_saver absl::random_internal_randen absl::raw_logging_internal absl::type_traits)
set(abseil_absl_random_internal_randen_engine_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_random_internal_randen_engine_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_random_internal_randen_engine_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_random_internal_randen_engine_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_random_internal_randen_engine_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_random_internal_randen_engine_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_random_internal_randen_engine_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_random_internal_randen_engine_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_random_internal_randen_engine_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::random_internal_mock_helpers VARIABLES ############################################

set(abseil_absl_random_internal_mock_helpers_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_random_internal_mock_helpers_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_random_internal_mock_helpers_RES_DIRS_RELEASE )
set(abseil_absl_random_internal_mock_helpers_DEFINITIONS_RELEASE )
set(abseil_absl_random_internal_mock_helpers_OBJECTS_RELEASE )
set(abseil_absl_random_internal_mock_helpers_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_random_internal_mock_helpers_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_random_internal_mock_helpers_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_random_internal_mock_helpers_LIBS_RELEASE )
set(abseil_absl_random_internal_mock_helpers_SYSTEM_LIBS_RELEASE )
set(abseil_absl_random_internal_mock_helpers_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_random_internal_mock_helpers_FRAMEWORKS_RELEASE )
set(abseil_absl_random_internal_mock_helpers_DEPENDENCIES_RELEASE absl::fast_type_id absl::optional)
set(abseil_absl_random_internal_mock_helpers_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_random_internal_mock_helpers_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_random_internal_mock_helpers_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_random_internal_mock_helpers_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_random_internal_mock_helpers_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_random_internal_mock_helpers_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_random_internal_mock_helpers_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_random_internal_mock_helpers_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_random_internal_mock_helpers_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::random_bit_gen_ref VARIABLES ############################################

set(abseil_absl_random_bit_gen_ref_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_random_bit_gen_ref_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_random_bit_gen_ref_RES_DIRS_RELEASE )
set(abseil_absl_random_bit_gen_ref_DEFINITIONS_RELEASE )
set(abseil_absl_random_bit_gen_ref_OBJECTS_RELEASE )
set(abseil_absl_random_bit_gen_ref_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_random_bit_gen_ref_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_random_bit_gen_ref_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_random_bit_gen_ref_LIBS_RELEASE )
set(abseil_absl_random_bit_gen_ref_SYSTEM_LIBS_RELEASE )
set(abseil_absl_random_bit_gen_ref_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_random_bit_gen_ref_FRAMEWORKS_RELEASE )
set(abseil_absl_random_bit_gen_ref_DEPENDENCIES_RELEASE absl::core_headers absl::random_internal_distribution_caller absl::random_internal_fast_uniform_bits absl::type_traits)
set(abseil_absl_random_bit_gen_ref_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_random_bit_gen_ref_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_random_bit_gen_ref_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_random_bit_gen_ref_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_random_bit_gen_ref_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_random_bit_gen_ref_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_random_bit_gen_ref_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_random_bit_gen_ref_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_random_bit_gen_ref_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::malloc_internal VARIABLES ############################################

set(abseil_absl_malloc_internal_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_malloc_internal_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_malloc_internal_RES_DIRS_RELEASE )
set(abseil_absl_malloc_internal_DEFINITIONS_RELEASE )
set(abseil_absl_malloc_internal_OBJECTS_RELEASE )
set(abseil_absl_malloc_internal_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_malloc_internal_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_malloc_internal_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_malloc_internal_LIBS_RELEASE absl_malloc_internal)
set(abseil_absl_malloc_internal_SYSTEM_LIBS_RELEASE )
set(abseil_absl_malloc_internal_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_malloc_internal_FRAMEWORKS_RELEASE )
set(abseil_absl_malloc_internal_DEPENDENCIES_RELEASE absl::base absl::base_internal absl::config absl::core_headers absl::dynamic_annotations absl::raw_logging_internal)
set(abseil_absl_malloc_internal_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_malloc_internal_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_malloc_internal_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_malloc_internal_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_malloc_internal_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_malloc_internal_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_malloc_internal_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_malloc_internal_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_malloc_internal_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::variant VARIABLES ############################################

set(abseil_absl_variant_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_variant_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_variant_RES_DIRS_RELEASE )
set(abseil_absl_variant_DEFINITIONS_RELEASE )
set(abseil_absl_variant_OBJECTS_RELEASE )
set(abseil_absl_variant_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_variant_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_variant_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_variant_LIBS_RELEASE )
set(abseil_absl_variant_SYSTEM_LIBS_RELEASE )
set(abseil_absl_variant_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_variant_FRAMEWORKS_RELEASE )
set(abseil_absl_variant_DEPENDENCIES_RELEASE absl::bad_variant_access absl::base_internal absl::config absl::core_headers absl::type_traits absl::utility)
set(abseil_absl_variant_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_variant_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_variant_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_variant_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_variant_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_variant_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_variant_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_variant_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_variant_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::optional VARIABLES ############################################

set(abseil_absl_optional_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_optional_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_optional_RES_DIRS_RELEASE )
set(abseil_absl_optional_DEFINITIONS_RELEASE )
set(abseil_absl_optional_OBJECTS_RELEASE )
set(abseil_absl_optional_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_optional_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_optional_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_optional_LIBS_RELEASE )
set(abseil_absl_optional_SYSTEM_LIBS_RELEASE )
set(abseil_absl_optional_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_optional_FRAMEWORKS_RELEASE )
set(abseil_absl_optional_DEPENDENCIES_RELEASE absl::bad_optional_access absl::base_internal absl::config absl::core_headers absl::memory absl::type_traits absl::utility)
set(abseil_absl_optional_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_optional_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_optional_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_optional_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_optional_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_optional_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_optional_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_optional_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_optional_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::bad_any_cast VARIABLES ############################################

set(abseil_absl_bad_any_cast_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_bad_any_cast_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_bad_any_cast_RES_DIRS_RELEASE )
set(abseil_absl_bad_any_cast_DEFINITIONS_RELEASE )
set(abseil_absl_bad_any_cast_OBJECTS_RELEASE )
set(abseil_absl_bad_any_cast_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_bad_any_cast_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_bad_any_cast_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_bad_any_cast_LIBS_RELEASE )
set(abseil_absl_bad_any_cast_SYSTEM_LIBS_RELEASE )
set(abseil_absl_bad_any_cast_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_bad_any_cast_FRAMEWORKS_RELEASE )
set(abseil_absl_bad_any_cast_DEPENDENCIES_RELEASE absl::bad_any_cast_impl absl::config)
set(abseil_absl_bad_any_cast_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_bad_any_cast_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_bad_any_cast_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_bad_any_cast_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_bad_any_cast_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_bad_any_cast_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_bad_any_cast_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_bad_any_cast_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_bad_any_cast_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::strings_internal VARIABLES ############################################

set(abseil_absl_strings_internal_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_strings_internal_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_strings_internal_RES_DIRS_RELEASE )
set(abseil_absl_strings_internal_DEFINITIONS_RELEASE )
set(abseil_absl_strings_internal_OBJECTS_RELEASE )
set(abseil_absl_strings_internal_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_strings_internal_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_strings_internal_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_strings_internal_LIBS_RELEASE absl_strings_internal)
set(abseil_absl_strings_internal_SYSTEM_LIBS_RELEASE )
set(abseil_absl_strings_internal_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_strings_internal_FRAMEWORKS_RELEASE )
set(abseil_absl_strings_internal_DEPENDENCIES_RELEASE absl::config absl::core_headers absl::endian absl::raw_logging_internal absl::type_traits)
set(abseil_absl_strings_internal_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_strings_internal_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_strings_internal_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_strings_internal_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_strings_internal_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_strings_internal_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_strings_internal_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_strings_internal_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_strings_internal_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::random_internal_randen VARIABLES ############################################

set(abseil_absl_random_internal_randen_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_random_internal_randen_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_random_internal_randen_RES_DIRS_RELEASE )
set(abseil_absl_random_internal_randen_DEFINITIONS_RELEASE )
set(abseil_absl_random_internal_randen_OBJECTS_RELEASE )
set(abseil_absl_random_internal_randen_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_random_internal_randen_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_random_internal_randen_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_random_internal_randen_LIBS_RELEASE absl_random_internal_randen)
set(abseil_absl_random_internal_randen_SYSTEM_LIBS_RELEASE )
set(abseil_absl_random_internal_randen_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_random_internal_randen_FRAMEWORKS_RELEASE )
set(abseil_absl_random_internal_randen_DEPENDENCIES_RELEASE absl::random_internal_platform absl::random_internal_randen_hwaes absl::random_internal_randen_slow)
set(abseil_absl_random_internal_randen_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_random_internal_randen_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_random_internal_randen_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_random_internal_randen_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_random_internal_randen_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_random_internal_randen_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_random_internal_randen_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_random_internal_randen_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_random_internal_randen_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::random_internal_distribution_caller VARIABLES ############################################

set(abseil_absl_random_internal_distribution_caller_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_random_internal_distribution_caller_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_random_internal_distribution_caller_RES_DIRS_RELEASE )
set(abseil_absl_random_internal_distribution_caller_DEFINITIONS_RELEASE )
set(abseil_absl_random_internal_distribution_caller_OBJECTS_RELEASE )
set(abseil_absl_random_internal_distribution_caller_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_random_internal_distribution_caller_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_random_internal_distribution_caller_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_random_internal_distribution_caller_LIBS_RELEASE )
set(abseil_absl_random_internal_distribution_caller_SYSTEM_LIBS_RELEASE )
set(abseil_absl_random_internal_distribution_caller_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_random_internal_distribution_caller_FRAMEWORKS_RELEASE )
set(abseil_absl_random_internal_distribution_caller_DEPENDENCIES_RELEASE absl::config absl::utility absl::fast_type_id)
set(abseil_absl_random_internal_distribution_caller_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_random_internal_distribution_caller_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_random_internal_distribution_caller_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_random_internal_distribution_caller_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_random_internal_distribution_caller_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_random_internal_distribution_caller_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_random_internal_distribution_caller_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_random_internal_distribution_caller_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_random_internal_distribution_caller_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::low_level_hash VARIABLES ############################################

set(abseil_absl_low_level_hash_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_low_level_hash_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_low_level_hash_RES_DIRS_RELEASE )
set(abseil_absl_low_level_hash_DEFINITIONS_RELEASE )
set(abseil_absl_low_level_hash_OBJECTS_RELEASE )
set(abseil_absl_low_level_hash_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_low_level_hash_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_low_level_hash_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_low_level_hash_LIBS_RELEASE absl_low_level_hash)
set(abseil_absl_low_level_hash_SYSTEM_LIBS_RELEASE )
set(abseil_absl_low_level_hash_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_low_level_hash_FRAMEWORKS_RELEASE )
set(abseil_absl_low_level_hash_DEPENDENCIES_RELEASE absl::bits absl::config absl::endian absl::int128)
set(abseil_absl_low_level_hash_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_low_level_hash_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_low_level_hash_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_low_level_hash_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_low_level_hash_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_low_level_hash_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_low_level_hash_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_low_level_hash_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_low_level_hash_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::city VARIABLES ############################################

set(abseil_absl_city_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_city_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_city_RES_DIRS_RELEASE )
set(abseil_absl_city_DEFINITIONS_RELEASE )
set(abseil_absl_city_OBJECTS_RELEASE )
set(abseil_absl_city_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_city_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_city_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_city_LIBS_RELEASE absl_city)
set(abseil_absl_city_SYSTEM_LIBS_RELEASE )
set(abseil_absl_city_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_city_FRAMEWORKS_RELEASE )
set(abseil_absl_city_DEPENDENCIES_RELEASE absl::config absl::core_headers absl::endian)
set(abseil_absl_city_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_city_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_city_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_city_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_city_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_city_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_city_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_city_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_city_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::bind_front VARIABLES ############################################

set(abseil_absl_bind_front_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_bind_front_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_bind_front_RES_DIRS_RELEASE )
set(abseil_absl_bind_front_DEFINITIONS_RELEASE )
set(abseil_absl_bind_front_OBJECTS_RELEASE )
set(abseil_absl_bind_front_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_bind_front_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_bind_front_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_bind_front_LIBS_RELEASE )
set(abseil_absl_bind_front_SYSTEM_LIBS_RELEASE )
set(abseil_absl_bind_front_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_bind_front_FRAMEWORKS_RELEASE )
set(abseil_absl_bind_front_DEPENDENCIES_RELEASE absl::base_internal absl::compressed_tuple)
set(abseil_absl_bind_front_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_bind_front_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_bind_front_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_bind_front_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_bind_front_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_bind_front_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_bind_front_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_bind_front_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_bind_front_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::any_invocable VARIABLES ############################################

set(abseil_absl_any_invocable_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_any_invocable_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_any_invocable_RES_DIRS_RELEASE )
set(abseil_absl_any_invocable_DEFINITIONS_RELEASE )
set(abseil_absl_any_invocable_OBJECTS_RELEASE )
set(abseil_absl_any_invocable_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_any_invocable_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_any_invocable_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_any_invocable_LIBS_RELEASE )
set(abseil_absl_any_invocable_SYSTEM_LIBS_RELEASE )
set(abseil_absl_any_invocable_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_any_invocable_FRAMEWORKS_RELEASE )
set(abseil_absl_any_invocable_DEPENDENCIES_RELEASE absl::base_internal absl::config absl::core_headers absl::type_traits absl::utility)
set(abseil_absl_any_invocable_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_any_invocable_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_any_invocable_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_any_invocable_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_any_invocable_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_any_invocable_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_any_invocable_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_any_invocable_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_any_invocable_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::debugging VARIABLES ############################################

set(abseil_absl_debugging_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_debugging_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_debugging_RES_DIRS_RELEASE )
set(abseil_absl_debugging_DEFINITIONS_RELEASE )
set(abseil_absl_debugging_OBJECTS_RELEASE )
set(abseil_absl_debugging_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_debugging_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_debugging_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_debugging_LIBS_RELEASE )
set(abseil_absl_debugging_SYSTEM_LIBS_RELEASE )
set(abseil_absl_debugging_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_debugging_FRAMEWORKS_RELEASE )
set(abseil_absl_debugging_DEPENDENCIES_RELEASE absl::stacktrace absl::leak_check)
set(abseil_absl_debugging_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_debugging_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_debugging_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_debugging_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_debugging_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_debugging_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_debugging_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_debugging_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_debugging_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::demangle_internal VARIABLES ############################################

set(abseil_absl_demangle_internal_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_demangle_internal_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_demangle_internal_RES_DIRS_RELEASE )
set(abseil_absl_demangle_internal_DEFINITIONS_RELEASE )
set(abseil_absl_demangle_internal_OBJECTS_RELEASE )
set(abseil_absl_demangle_internal_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_demangle_internal_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_demangle_internal_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_demangle_internal_LIBS_RELEASE absl_demangle_internal)
set(abseil_absl_demangle_internal_SYSTEM_LIBS_RELEASE )
set(abseil_absl_demangle_internal_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_demangle_internal_FRAMEWORKS_RELEASE )
set(abseil_absl_demangle_internal_DEPENDENCIES_RELEASE absl::base absl::core_headers)
set(abseil_absl_demangle_internal_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_demangle_internal_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_demangle_internal_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_demangle_internal_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_demangle_internal_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_demangle_internal_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_demangle_internal_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_demangle_internal_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_demangle_internal_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::stacktrace VARIABLES ############################################

set(abseil_absl_stacktrace_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_stacktrace_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_stacktrace_RES_DIRS_RELEASE )
set(abseil_absl_stacktrace_DEFINITIONS_RELEASE )
set(abseil_absl_stacktrace_OBJECTS_RELEASE )
set(abseil_absl_stacktrace_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_stacktrace_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_stacktrace_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_stacktrace_LIBS_RELEASE absl_stacktrace)
set(abseil_absl_stacktrace_SYSTEM_LIBS_RELEASE )
set(abseil_absl_stacktrace_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_stacktrace_FRAMEWORKS_RELEASE )
set(abseil_absl_stacktrace_DEPENDENCIES_RELEASE absl::debugging_internal absl::config absl::core_headers)
set(abseil_absl_stacktrace_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_stacktrace_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_stacktrace_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_stacktrace_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_stacktrace_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_stacktrace_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_stacktrace_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_stacktrace_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_stacktrace_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::container_memory VARIABLES ############################################

set(abseil_absl_container_memory_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_container_memory_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_container_memory_RES_DIRS_RELEASE )
set(abseil_absl_container_memory_DEFINITIONS_RELEASE )
set(abseil_absl_container_memory_OBJECTS_RELEASE )
set(abseil_absl_container_memory_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_container_memory_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_container_memory_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_container_memory_LIBS_RELEASE )
set(abseil_absl_container_memory_SYSTEM_LIBS_RELEASE )
set(abseil_absl_container_memory_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_container_memory_FRAMEWORKS_RELEASE )
set(abseil_absl_container_memory_DEPENDENCIES_RELEASE absl::config absl::memory absl::type_traits absl::utility)
set(abseil_absl_container_memory_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_container_memory_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_container_memory_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_container_memory_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_container_memory_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_container_memory_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_container_memory_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_container_memory_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_container_memory_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::inlined_vector VARIABLES ############################################

set(abseil_absl_inlined_vector_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_inlined_vector_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_inlined_vector_RES_DIRS_RELEASE )
set(abseil_absl_inlined_vector_DEFINITIONS_RELEASE )
set(abseil_absl_inlined_vector_OBJECTS_RELEASE )
set(abseil_absl_inlined_vector_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_inlined_vector_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_inlined_vector_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_inlined_vector_LIBS_RELEASE )
set(abseil_absl_inlined_vector_SYSTEM_LIBS_RELEASE )
set(abseil_absl_inlined_vector_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_inlined_vector_FRAMEWORKS_RELEASE )
set(abseil_absl_inlined_vector_DEPENDENCIES_RELEASE absl::algorithm absl::core_headers absl::inlined_vector_internal absl::throw_delegate absl::memory)
set(abseil_absl_inlined_vector_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_inlined_vector_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_inlined_vector_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_inlined_vector_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_inlined_vector_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_inlined_vector_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_inlined_vector_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_inlined_vector_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_inlined_vector_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::inlined_vector_internal VARIABLES ############################################

set(abseil_absl_inlined_vector_internal_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_inlined_vector_internal_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_inlined_vector_internal_RES_DIRS_RELEASE )
set(abseil_absl_inlined_vector_internal_DEFINITIONS_RELEASE )
set(abseil_absl_inlined_vector_internal_OBJECTS_RELEASE )
set(abseil_absl_inlined_vector_internal_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_inlined_vector_internal_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_inlined_vector_internal_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_inlined_vector_internal_LIBS_RELEASE )
set(abseil_absl_inlined_vector_internal_SYSTEM_LIBS_RELEASE )
set(abseil_absl_inlined_vector_internal_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_inlined_vector_internal_FRAMEWORKS_RELEASE )
set(abseil_absl_inlined_vector_internal_DEPENDENCIES_RELEASE absl::compressed_tuple absl::core_headers absl::memory absl::span absl::type_traits)
set(abseil_absl_inlined_vector_internal_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_inlined_vector_internal_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_inlined_vector_internal_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_inlined_vector_internal_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_inlined_vector_internal_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_inlined_vector_internal_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_inlined_vector_internal_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_inlined_vector_internal_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_inlined_vector_internal_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::fixed_array VARIABLES ############################################

set(abseil_absl_fixed_array_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_fixed_array_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_fixed_array_RES_DIRS_RELEASE )
set(abseil_absl_fixed_array_DEFINITIONS_RELEASE )
set(abseil_absl_fixed_array_OBJECTS_RELEASE )
set(abseil_absl_fixed_array_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_fixed_array_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_fixed_array_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_fixed_array_LIBS_RELEASE )
set(abseil_absl_fixed_array_SYSTEM_LIBS_RELEASE )
set(abseil_absl_fixed_array_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_fixed_array_FRAMEWORKS_RELEASE )
set(abseil_absl_fixed_array_DEPENDENCIES_RELEASE absl::compressed_tuple absl::algorithm absl::config absl::core_headers absl::dynamic_annotations absl::throw_delegate absl::memory)
set(abseil_absl_fixed_array_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_fixed_array_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_fixed_array_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_fixed_array_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_fixed_array_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_fixed_array_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_fixed_array_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_fixed_array_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_fixed_array_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::compressed_tuple VARIABLES ############################################

set(abseil_absl_compressed_tuple_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_compressed_tuple_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_compressed_tuple_RES_DIRS_RELEASE )
set(abseil_absl_compressed_tuple_DEFINITIONS_RELEASE )
set(abseil_absl_compressed_tuple_OBJECTS_RELEASE )
set(abseil_absl_compressed_tuple_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_compressed_tuple_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_compressed_tuple_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_compressed_tuple_LIBS_RELEASE )
set(abseil_absl_compressed_tuple_SYSTEM_LIBS_RELEASE )
set(abseil_absl_compressed_tuple_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_compressed_tuple_FRAMEWORKS_RELEASE )
set(abseil_absl_compressed_tuple_DEPENDENCIES_RELEASE absl::utility)
set(abseil_absl_compressed_tuple_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_compressed_tuple_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_compressed_tuple_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_compressed_tuple_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_compressed_tuple_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_compressed_tuple_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_compressed_tuple_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_compressed_tuple_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_compressed_tuple_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::cleanup VARIABLES ############################################

set(abseil_absl_cleanup_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_cleanup_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_cleanup_RES_DIRS_RELEASE )
set(abseil_absl_cleanup_DEFINITIONS_RELEASE )
set(abseil_absl_cleanup_OBJECTS_RELEASE )
set(abseil_absl_cleanup_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_cleanup_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_cleanup_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_cleanup_LIBS_RELEASE )
set(abseil_absl_cleanup_SYSTEM_LIBS_RELEASE )
set(abseil_absl_cleanup_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_cleanup_FRAMEWORKS_RELEASE )
set(abseil_absl_cleanup_DEPENDENCIES_RELEASE absl::cleanup_internal absl::config absl::core_headers)
set(abseil_absl_cleanup_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_cleanup_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_cleanup_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_cleanup_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_cleanup_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_cleanup_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_cleanup_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_cleanup_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_cleanup_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::cleanup_internal VARIABLES ############################################

set(abseil_absl_cleanup_internal_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_cleanup_internal_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_cleanup_internal_RES_DIRS_RELEASE )
set(abseil_absl_cleanup_internal_DEFINITIONS_RELEASE )
set(abseil_absl_cleanup_internal_OBJECTS_RELEASE )
set(abseil_absl_cleanup_internal_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_cleanup_internal_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_cleanup_internal_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_cleanup_internal_LIBS_RELEASE )
set(abseil_absl_cleanup_internal_SYSTEM_LIBS_RELEASE )
set(abseil_absl_cleanup_internal_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_cleanup_internal_FRAMEWORKS_RELEASE )
set(abseil_absl_cleanup_internal_DEPENDENCIES_RELEASE absl::base_internal absl::core_headers absl::utility)
set(abseil_absl_cleanup_internal_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_cleanup_internal_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_cleanup_internal_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_cleanup_internal_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_cleanup_internal_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_cleanup_internal_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_cleanup_internal_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_cleanup_internal_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_cleanup_internal_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::endian VARIABLES ############################################

set(abseil_absl_endian_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_endian_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_endian_RES_DIRS_RELEASE )
set(abseil_absl_endian_DEFINITIONS_RELEASE )
set(abseil_absl_endian_OBJECTS_RELEASE )
set(abseil_absl_endian_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_endian_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_endian_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_endian_LIBS_RELEASE )
set(abseil_absl_endian_SYSTEM_LIBS_RELEASE )
set(abseil_absl_endian_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_endian_FRAMEWORKS_RELEASE )
set(abseil_absl_endian_DEPENDENCIES_RELEASE absl::base absl::config absl::core_headers)
set(abseil_absl_endian_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_endian_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_endian_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_endian_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_endian_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_endian_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_endian_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_endian_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_endian_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::base VARIABLES ############################################

set(abseil_absl_base_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_base_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_base_RES_DIRS_RELEASE )
set(abseil_absl_base_DEFINITIONS_RELEASE )
set(abseil_absl_base_OBJECTS_RELEASE )
set(abseil_absl_base_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_base_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_base_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_base_LIBS_RELEASE absl_base)
set(abseil_absl_base_SYSTEM_LIBS_RELEASE )
set(abseil_absl_base_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_base_FRAMEWORKS_RELEASE )
set(abseil_absl_base_DEPENDENCIES_RELEASE absl::atomic_hook absl::base_internal absl::config absl::core_headers absl::dynamic_annotations absl::log_severity absl::raw_logging_internal absl::spinlock_wait absl::type_traits)
set(abseil_absl_base_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_base_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_base_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_base_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_base_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_base_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_base_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_base_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_base_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::spinlock_wait VARIABLES ############################################

set(abseil_absl_spinlock_wait_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_spinlock_wait_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_spinlock_wait_RES_DIRS_RELEASE )
set(abseil_absl_spinlock_wait_DEFINITIONS_RELEASE )
set(abseil_absl_spinlock_wait_OBJECTS_RELEASE )
set(abseil_absl_spinlock_wait_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_spinlock_wait_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_spinlock_wait_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_spinlock_wait_LIBS_RELEASE absl_spinlock_wait)
set(abseil_absl_spinlock_wait_SYSTEM_LIBS_RELEASE )
set(abseil_absl_spinlock_wait_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_spinlock_wait_FRAMEWORKS_RELEASE )
set(abseil_absl_spinlock_wait_DEPENDENCIES_RELEASE absl::base_internal absl::core_headers absl::errno_saver)
set(abseil_absl_spinlock_wait_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_spinlock_wait_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_spinlock_wait_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_spinlock_wait_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_spinlock_wait_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_spinlock_wait_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_spinlock_wait_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_spinlock_wait_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_spinlock_wait_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::utility VARIABLES ############################################

set(abseil_absl_utility_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_utility_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_utility_RES_DIRS_RELEASE )
set(abseil_absl_utility_DEFINITIONS_RELEASE )
set(abseil_absl_utility_OBJECTS_RELEASE )
set(abseil_absl_utility_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_utility_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_utility_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_utility_LIBS_RELEASE )
set(abseil_absl_utility_SYSTEM_LIBS_RELEASE )
set(abseil_absl_utility_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_utility_FRAMEWORKS_RELEASE )
set(abseil_absl_utility_DEPENDENCIES_RELEASE absl::base_internal absl::config absl::type_traits)
set(abseil_absl_utility_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_utility_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_utility_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_utility_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_utility_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_utility_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_utility_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_utility_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_utility_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::bad_variant_access VARIABLES ############################################

set(abseil_absl_bad_variant_access_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_bad_variant_access_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_bad_variant_access_RES_DIRS_RELEASE )
set(abseil_absl_bad_variant_access_DEFINITIONS_RELEASE )
set(abseil_absl_bad_variant_access_OBJECTS_RELEASE )
set(abseil_absl_bad_variant_access_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_bad_variant_access_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_bad_variant_access_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_bad_variant_access_LIBS_RELEASE absl_bad_variant_access)
set(abseil_absl_bad_variant_access_SYSTEM_LIBS_RELEASE )
set(abseil_absl_bad_variant_access_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_bad_variant_access_FRAMEWORKS_RELEASE )
set(abseil_absl_bad_variant_access_DEPENDENCIES_RELEASE absl::config absl::raw_logging_internal)
set(abseil_absl_bad_variant_access_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_bad_variant_access_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_bad_variant_access_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_bad_variant_access_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_bad_variant_access_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_bad_variant_access_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_bad_variant_access_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_bad_variant_access_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_bad_variant_access_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::bad_optional_access VARIABLES ############################################

set(abseil_absl_bad_optional_access_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_bad_optional_access_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_bad_optional_access_RES_DIRS_RELEASE )
set(abseil_absl_bad_optional_access_DEFINITIONS_RELEASE )
set(abseil_absl_bad_optional_access_OBJECTS_RELEASE )
set(abseil_absl_bad_optional_access_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_bad_optional_access_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_bad_optional_access_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_bad_optional_access_LIBS_RELEASE absl_bad_optional_access)
set(abseil_absl_bad_optional_access_SYSTEM_LIBS_RELEASE )
set(abseil_absl_bad_optional_access_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_bad_optional_access_FRAMEWORKS_RELEASE )
set(abseil_absl_bad_optional_access_DEPENDENCIES_RELEASE absl::config absl::raw_logging_internal)
set(abseil_absl_bad_optional_access_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_bad_optional_access_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_bad_optional_access_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_bad_optional_access_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_bad_optional_access_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_bad_optional_access_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_bad_optional_access_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_bad_optional_access_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_bad_optional_access_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::span VARIABLES ############################################

set(abseil_absl_span_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_span_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_span_RES_DIRS_RELEASE )
set(abseil_absl_span_DEFINITIONS_RELEASE )
set(abseil_absl_span_OBJECTS_RELEASE )
set(abseil_absl_span_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_span_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_span_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_span_LIBS_RELEASE )
set(abseil_absl_span_SYSTEM_LIBS_RELEASE )
set(abseil_absl_span_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_span_FRAMEWORKS_RELEASE )
set(abseil_absl_span_DEPENDENCIES_RELEASE absl::algorithm absl::core_headers absl::throw_delegate absl::type_traits)
set(abseil_absl_span_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_span_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_span_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_span_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_span_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_span_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_span_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_span_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_span_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::bad_any_cast_impl VARIABLES ############################################

set(abseil_absl_bad_any_cast_impl_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_bad_any_cast_impl_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_bad_any_cast_impl_RES_DIRS_RELEASE )
set(abseil_absl_bad_any_cast_impl_DEFINITIONS_RELEASE )
set(abseil_absl_bad_any_cast_impl_OBJECTS_RELEASE )
set(abseil_absl_bad_any_cast_impl_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_bad_any_cast_impl_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_bad_any_cast_impl_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_bad_any_cast_impl_LIBS_RELEASE absl_bad_any_cast_impl)
set(abseil_absl_bad_any_cast_impl_SYSTEM_LIBS_RELEASE )
set(abseil_absl_bad_any_cast_impl_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_bad_any_cast_impl_FRAMEWORKS_RELEASE )
set(abseil_absl_bad_any_cast_impl_DEPENDENCIES_RELEASE absl::config absl::raw_logging_internal)
set(abseil_absl_bad_any_cast_impl_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_bad_any_cast_impl_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_bad_any_cast_impl_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_bad_any_cast_impl_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_bad_any_cast_impl_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_bad_any_cast_impl_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_bad_any_cast_impl_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_bad_any_cast_impl_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_bad_any_cast_impl_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::cordz_functions VARIABLES ############################################

set(abseil_absl_cordz_functions_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_cordz_functions_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_cordz_functions_RES_DIRS_RELEASE )
set(abseil_absl_cordz_functions_DEFINITIONS_RELEASE )
set(abseil_absl_cordz_functions_OBJECTS_RELEASE )
set(abseil_absl_cordz_functions_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_cordz_functions_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_cordz_functions_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_cordz_functions_LIBS_RELEASE absl_cordz_functions)
set(abseil_absl_cordz_functions_SYSTEM_LIBS_RELEASE )
set(abseil_absl_cordz_functions_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_cordz_functions_FRAMEWORKS_RELEASE )
set(abseil_absl_cordz_functions_DEPENDENCIES_RELEASE absl::config absl::core_headers absl::exponential_biased absl::raw_logging_internal)
set(abseil_absl_cordz_functions_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_cordz_functions_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_cordz_functions_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_cordz_functions_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_cordz_functions_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_cordz_functions_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_cordz_functions_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_cordz_functions_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_cordz_functions_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::random_internal_randen_hwaes VARIABLES ############################################

set(abseil_absl_random_internal_randen_hwaes_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_random_internal_randen_hwaes_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_random_internal_randen_hwaes_RES_DIRS_RELEASE )
set(abseil_absl_random_internal_randen_hwaes_DEFINITIONS_RELEASE )
set(abseil_absl_random_internal_randen_hwaes_OBJECTS_RELEASE )
set(abseil_absl_random_internal_randen_hwaes_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_random_internal_randen_hwaes_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_random_internal_randen_hwaes_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_random_internal_randen_hwaes_LIBS_RELEASE absl_random_internal_randen_hwaes)
set(abseil_absl_random_internal_randen_hwaes_SYSTEM_LIBS_RELEASE )
set(abseil_absl_random_internal_randen_hwaes_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_random_internal_randen_hwaes_FRAMEWORKS_RELEASE )
set(abseil_absl_random_internal_randen_hwaes_DEPENDENCIES_RELEASE absl::random_internal_platform absl::random_internal_randen_hwaes_impl absl::config)
set(abseil_absl_random_internal_randen_hwaes_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_random_internal_randen_hwaes_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_random_internal_randen_hwaes_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_random_internal_randen_hwaes_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_random_internal_randen_hwaes_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_random_internal_randen_hwaes_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_random_internal_randen_hwaes_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_random_internal_randen_hwaes_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_random_internal_randen_hwaes_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::random_internal_generate_real VARIABLES ############################################

set(abseil_absl_random_internal_generate_real_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_random_internal_generate_real_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_random_internal_generate_real_RES_DIRS_RELEASE )
set(abseil_absl_random_internal_generate_real_DEFINITIONS_RELEASE )
set(abseil_absl_random_internal_generate_real_OBJECTS_RELEASE )
set(abseil_absl_random_internal_generate_real_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_random_internal_generate_real_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_random_internal_generate_real_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_random_internal_generate_real_LIBS_RELEASE )
set(abseil_absl_random_internal_generate_real_SYSTEM_LIBS_RELEASE )
set(abseil_absl_random_internal_generate_real_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_random_internal_generate_real_FRAMEWORKS_RELEASE )
set(abseil_absl_random_internal_generate_real_DEPENDENCIES_RELEASE absl::bits absl::random_internal_fastmath absl::random_internal_traits absl::type_traits)
set(abseil_absl_random_internal_generate_real_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_random_internal_generate_real_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_random_internal_generate_real_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_random_internal_generate_real_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_random_internal_generate_real_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_random_internal_generate_real_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_random_internal_generate_real_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_random_internal_generate_real_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_random_internal_generate_real_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::memory VARIABLES ############################################

set(abseil_absl_memory_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_memory_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_memory_RES_DIRS_RELEASE )
set(abseil_absl_memory_DEFINITIONS_RELEASE )
set(abseil_absl_memory_OBJECTS_RELEASE )
set(abseil_absl_memory_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_memory_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_memory_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_memory_LIBS_RELEASE )
set(abseil_absl_memory_SYSTEM_LIBS_RELEASE )
set(abseil_absl_memory_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_memory_FRAMEWORKS_RELEASE )
set(abseil_absl_memory_DEPENDENCIES_RELEASE absl::core_headers absl::meta)
set(abseil_absl_memory_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_memory_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_memory_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_memory_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_memory_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_memory_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_memory_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_memory_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_memory_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::function_ref VARIABLES ############################################

set(abseil_absl_function_ref_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_function_ref_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_function_ref_RES_DIRS_RELEASE )
set(abseil_absl_function_ref_DEFINITIONS_RELEASE )
set(abseil_absl_function_ref_OBJECTS_RELEASE )
set(abseil_absl_function_ref_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_function_ref_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_function_ref_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_function_ref_LIBS_RELEASE )
set(abseil_absl_function_ref_SYSTEM_LIBS_RELEASE )
set(abseil_absl_function_ref_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_function_ref_FRAMEWORKS_RELEASE )
set(abseil_absl_function_ref_DEPENDENCIES_RELEASE absl::base_internal absl::core_headers absl::meta)
set(abseil_absl_function_ref_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_function_ref_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_function_ref_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_function_ref_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_function_ref_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_function_ref_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_function_ref_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_function_ref_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_function_ref_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::debugging_internal VARIABLES ############################################

set(abseil_absl_debugging_internal_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_debugging_internal_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_debugging_internal_RES_DIRS_RELEASE )
set(abseil_absl_debugging_internal_DEFINITIONS_RELEASE )
set(abseil_absl_debugging_internal_OBJECTS_RELEASE )
set(abseil_absl_debugging_internal_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_debugging_internal_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_debugging_internal_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_debugging_internal_LIBS_RELEASE absl_debugging_internal)
set(abseil_absl_debugging_internal_SYSTEM_LIBS_RELEASE )
set(abseil_absl_debugging_internal_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_debugging_internal_FRAMEWORKS_RELEASE )
set(abseil_absl_debugging_internal_DEPENDENCIES_RELEASE absl::core_headers absl::config absl::dynamic_annotations absl::errno_saver absl::raw_logging_internal)
set(abseil_absl_debugging_internal_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_debugging_internal_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_debugging_internal_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_debugging_internal_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_debugging_internal_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_debugging_internal_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_debugging_internal_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_debugging_internal_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_debugging_internal_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::container_common VARIABLES ############################################

set(abseil_absl_container_common_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_container_common_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_container_common_RES_DIRS_RELEASE )
set(abseil_absl_container_common_DEFINITIONS_RELEASE )
set(abseil_absl_container_common_OBJECTS_RELEASE )
set(abseil_absl_container_common_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_container_common_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_container_common_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_container_common_LIBS_RELEASE )
set(abseil_absl_container_common_SYSTEM_LIBS_RELEASE )
set(abseil_absl_container_common_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_container_common_FRAMEWORKS_RELEASE )
set(abseil_absl_container_common_DEPENDENCIES_RELEASE absl::type_traits)
set(abseil_absl_container_common_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_container_common_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_container_common_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_container_common_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_container_common_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_container_common_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_container_common_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_container_common_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_container_common_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::hashtable_debug VARIABLES ############################################

set(abseil_absl_hashtable_debug_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_hashtable_debug_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_hashtable_debug_RES_DIRS_RELEASE )
set(abseil_absl_hashtable_debug_DEFINITIONS_RELEASE )
set(abseil_absl_hashtable_debug_OBJECTS_RELEASE )
set(abseil_absl_hashtable_debug_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_hashtable_debug_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_hashtable_debug_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_hashtable_debug_LIBS_RELEASE )
set(abseil_absl_hashtable_debug_SYSTEM_LIBS_RELEASE )
set(abseil_absl_hashtable_debug_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_hashtable_debug_FRAMEWORKS_RELEASE )
set(abseil_absl_hashtable_debug_DEPENDENCIES_RELEASE absl::hashtable_debug_hooks)
set(abseil_absl_hashtable_debug_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_hashtable_debug_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_hashtable_debug_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_hashtable_debug_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_hashtable_debug_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_hashtable_debug_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_hashtable_debug_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_hashtable_debug_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_hashtable_debug_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::hash_policy_traits VARIABLES ############################################

set(abseil_absl_hash_policy_traits_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_hash_policy_traits_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_hash_policy_traits_RES_DIRS_RELEASE )
set(abseil_absl_hash_policy_traits_DEFINITIONS_RELEASE )
set(abseil_absl_hash_policy_traits_OBJECTS_RELEASE )
set(abseil_absl_hash_policy_traits_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_hash_policy_traits_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_hash_policy_traits_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_hash_policy_traits_LIBS_RELEASE )
set(abseil_absl_hash_policy_traits_SYSTEM_LIBS_RELEASE )
set(abseil_absl_hash_policy_traits_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_hash_policy_traits_FRAMEWORKS_RELEASE )
set(abseil_absl_hash_policy_traits_DEPENDENCIES_RELEASE absl::meta)
set(abseil_absl_hash_policy_traits_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_hash_policy_traits_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_hash_policy_traits_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_hash_policy_traits_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_hash_policy_traits_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_hash_policy_traits_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_hash_policy_traits_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_hash_policy_traits_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_hash_policy_traits_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::algorithm_container VARIABLES ############################################

set(abseil_absl_algorithm_container_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_algorithm_container_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_algorithm_container_RES_DIRS_RELEASE )
set(abseil_absl_algorithm_container_DEFINITIONS_RELEASE )
set(abseil_absl_algorithm_container_OBJECTS_RELEASE )
set(abseil_absl_algorithm_container_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_algorithm_container_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_algorithm_container_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_algorithm_container_LIBS_RELEASE )
set(abseil_absl_algorithm_container_SYSTEM_LIBS_RELEASE )
set(abseil_absl_algorithm_container_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_algorithm_container_FRAMEWORKS_RELEASE )
set(abseil_absl_algorithm_container_DEPENDENCIES_RELEASE absl::algorithm absl::core_headers absl::meta)
set(abseil_absl_algorithm_container_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_algorithm_container_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_algorithm_container_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_algorithm_container_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_algorithm_container_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_algorithm_container_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_algorithm_container_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_algorithm_container_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_algorithm_container_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::strerror VARIABLES ############################################

set(abseil_absl_strerror_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_strerror_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_strerror_RES_DIRS_RELEASE )
set(abseil_absl_strerror_DEFINITIONS_RELEASE )
set(abseil_absl_strerror_OBJECTS_RELEASE )
set(abseil_absl_strerror_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_strerror_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_strerror_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_strerror_LIBS_RELEASE absl_strerror)
set(abseil_absl_strerror_SYSTEM_LIBS_RELEASE )
set(abseil_absl_strerror_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_strerror_FRAMEWORKS_RELEASE )
set(abseil_absl_strerror_DEPENDENCIES_RELEASE absl::config absl::core_headers absl::errno_saver)
set(abseil_absl_strerror_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_strerror_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_strerror_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_strerror_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_strerror_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_strerror_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_strerror_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_strerror_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_strerror_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::scoped_set_env VARIABLES ############################################

set(abseil_absl_scoped_set_env_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_scoped_set_env_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_scoped_set_env_RES_DIRS_RELEASE )
set(abseil_absl_scoped_set_env_DEFINITIONS_RELEASE )
set(abseil_absl_scoped_set_env_OBJECTS_RELEASE )
set(abseil_absl_scoped_set_env_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_scoped_set_env_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_scoped_set_env_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_scoped_set_env_LIBS_RELEASE absl_scoped_set_env)
set(abseil_absl_scoped_set_env_SYSTEM_LIBS_RELEASE )
set(abseil_absl_scoped_set_env_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_scoped_set_env_FRAMEWORKS_RELEASE )
set(abseil_absl_scoped_set_env_DEPENDENCIES_RELEASE absl::config absl::raw_logging_internal)
set(abseil_absl_scoped_set_env_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_scoped_set_env_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_scoped_set_env_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_scoped_set_env_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_scoped_set_env_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_scoped_set_env_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_scoped_set_env_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_scoped_set_env_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_scoped_set_env_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::throw_delegate VARIABLES ############################################

set(abseil_absl_throw_delegate_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_throw_delegate_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_throw_delegate_RES_DIRS_RELEASE )
set(abseil_absl_throw_delegate_DEFINITIONS_RELEASE )
set(abseil_absl_throw_delegate_OBJECTS_RELEASE )
set(abseil_absl_throw_delegate_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_throw_delegate_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_throw_delegate_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_throw_delegate_LIBS_RELEASE absl_throw_delegate)
set(abseil_absl_throw_delegate_SYSTEM_LIBS_RELEASE )
set(abseil_absl_throw_delegate_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_throw_delegate_FRAMEWORKS_RELEASE )
set(abseil_absl_throw_delegate_DEPENDENCIES_RELEASE absl::config absl::raw_logging_internal)
set(abseil_absl_throw_delegate_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_throw_delegate_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_throw_delegate_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_throw_delegate_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_throw_delegate_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_throw_delegate_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_throw_delegate_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_throw_delegate_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_throw_delegate_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::base_internal VARIABLES ############################################

set(abseil_absl_base_internal_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_base_internal_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_base_internal_RES_DIRS_RELEASE )
set(abseil_absl_base_internal_DEFINITIONS_RELEASE )
set(abseil_absl_base_internal_OBJECTS_RELEASE )
set(abseil_absl_base_internal_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_base_internal_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_base_internal_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_base_internal_LIBS_RELEASE )
set(abseil_absl_base_internal_SYSTEM_LIBS_RELEASE )
set(abseil_absl_base_internal_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_base_internal_FRAMEWORKS_RELEASE )
set(abseil_absl_base_internal_DEPENDENCIES_RELEASE absl::config absl::type_traits)
set(abseil_absl_base_internal_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_base_internal_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_base_internal_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_base_internal_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_base_internal_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_base_internal_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_base_internal_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_base_internal_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_base_internal_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::raw_logging_internal VARIABLES ############################################

set(abseil_absl_raw_logging_internal_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_raw_logging_internal_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_raw_logging_internal_RES_DIRS_RELEASE )
set(abseil_absl_raw_logging_internal_DEFINITIONS_RELEASE )
set(abseil_absl_raw_logging_internal_OBJECTS_RELEASE )
set(abseil_absl_raw_logging_internal_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_raw_logging_internal_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_raw_logging_internal_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_raw_logging_internal_LIBS_RELEASE absl_raw_logging_internal)
set(abseil_absl_raw_logging_internal_SYSTEM_LIBS_RELEASE )
set(abseil_absl_raw_logging_internal_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_raw_logging_internal_FRAMEWORKS_RELEASE )
set(abseil_absl_raw_logging_internal_DEPENDENCIES_RELEASE absl::atomic_hook absl::config absl::core_headers absl::errno_saver absl::log_severity)
set(abseil_absl_raw_logging_internal_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_raw_logging_internal_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_raw_logging_internal_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_raw_logging_internal_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_raw_logging_internal_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_raw_logging_internal_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_raw_logging_internal_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_raw_logging_internal_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_raw_logging_internal_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::log_severity VARIABLES ############################################

set(abseil_absl_log_severity_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_log_severity_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_log_severity_RES_DIRS_RELEASE )
set(abseil_absl_log_severity_DEFINITIONS_RELEASE )
set(abseil_absl_log_severity_OBJECTS_RELEASE )
set(abseil_absl_log_severity_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_log_severity_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_log_severity_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_log_severity_LIBS_RELEASE absl_log_severity)
set(abseil_absl_log_severity_SYSTEM_LIBS_RELEASE )
set(abseil_absl_log_severity_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_log_severity_FRAMEWORKS_RELEASE )
set(abseil_absl_log_severity_DEPENDENCIES_RELEASE absl::core_headers)
set(abseil_absl_log_severity_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_log_severity_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_log_severity_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_log_severity_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_log_severity_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_log_severity_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_log_severity_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_log_severity_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_log_severity_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::errno_saver VARIABLES ############################################

set(abseil_absl_errno_saver_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_errno_saver_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_errno_saver_RES_DIRS_RELEASE )
set(abseil_absl_errno_saver_DEFINITIONS_RELEASE )
set(abseil_absl_errno_saver_OBJECTS_RELEASE )
set(abseil_absl_errno_saver_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_errno_saver_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_errno_saver_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_errno_saver_LIBS_RELEASE )
set(abseil_absl_errno_saver_SYSTEM_LIBS_RELEASE )
set(abseil_absl_errno_saver_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_errno_saver_FRAMEWORKS_RELEASE )
set(abseil_absl_errno_saver_DEPENDENCIES_RELEASE absl::config)
set(abseil_absl_errno_saver_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_errno_saver_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_errno_saver_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_errno_saver_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_errno_saver_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_errno_saver_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_errno_saver_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_errno_saver_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_errno_saver_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::atomic_hook VARIABLES ############################################

set(abseil_absl_atomic_hook_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_atomic_hook_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_atomic_hook_RES_DIRS_RELEASE )
set(abseil_absl_atomic_hook_DEFINITIONS_RELEASE )
set(abseil_absl_atomic_hook_OBJECTS_RELEASE )
set(abseil_absl_atomic_hook_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_atomic_hook_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_atomic_hook_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_atomic_hook_LIBS_RELEASE )
set(abseil_absl_atomic_hook_SYSTEM_LIBS_RELEASE )
set(abseil_absl_atomic_hook_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_atomic_hook_FRAMEWORKS_RELEASE )
set(abseil_absl_atomic_hook_DEPENDENCIES_RELEASE absl::config absl::core_headers)
set(abseil_absl_atomic_hook_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_atomic_hook_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_atomic_hook_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_atomic_hook_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_atomic_hook_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_atomic_hook_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_atomic_hook_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_atomic_hook_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_atomic_hook_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::compare VARIABLES ############################################

set(abseil_absl_compare_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_compare_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_compare_RES_DIRS_RELEASE )
set(abseil_absl_compare_DEFINITIONS_RELEASE )
set(abseil_absl_compare_OBJECTS_RELEASE )
set(abseil_absl_compare_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_compare_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_compare_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_compare_LIBS_RELEASE )
set(abseil_absl_compare_SYSTEM_LIBS_RELEASE )
set(abseil_absl_compare_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_compare_FRAMEWORKS_RELEASE )
set(abseil_absl_compare_DEPENDENCIES_RELEASE absl::core_headers absl::type_traits)
set(abseil_absl_compare_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_compare_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_compare_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_compare_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_compare_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_compare_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_compare_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_compare_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_compare_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::time_zone VARIABLES ############################################

set(abseil_absl_time_zone_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_time_zone_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_time_zone_RES_DIRS_RELEASE )
set(abseil_absl_time_zone_DEFINITIONS_RELEASE )
set(abseil_absl_time_zone_OBJECTS_RELEASE )
set(abseil_absl_time_zone_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_time_zone_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_time_zone_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_time_zone_LIBS_RELEASE absl_time_zone)
set(abseil_absl_time_zone_SYSTEM_LIBS_RELEASE )
set(abseil_absl_time_zone_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_time_zone_FRAMEWORKS_RELEASE CoreFoundation)
set(abseil_absl_time_zone_DEPENDENCIES_RELEASE )
set(abseil_absl_time_zone_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_time_zone_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_time_zone_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_time_zone_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_time_zone_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_time_zone_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_time_zone_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_time_zone_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_time_zone_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::civil_time VARIABLES ############################################

set(abseil_absl_civil_time_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_civil_time_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_civil_time_RES_DIRS_RELEASE )
set(abseil_absl_civil_time_DEFINITIONS_RELEASE )
set(abseil_absl_civil_time_OBJECTS_RELEASE )
set(abseil_absl_civil_time_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_civil_time_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_civil_time_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_civil_time_LIBS_RELEASE absl_civil_time)
set(abseil_absl_civil_time_SYSTEM_LIBS_RELEASE )
set(abseil_absl_civil_time_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_civil_time_FRAMEWORKS_RELEASE )
set(abseil_absl_civil_time_DEPENDENCIES_RELEASE )
set(abseil_absl_civil_time_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_civil_time_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_civil_time_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_civil_time_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_civil_time_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_civil_time_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_civil_time_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_civil_time_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_civil_time_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::cordz_update_tracker VARIABLES ############################################

set(abseil_absl_cordz_update_tracker_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_cordz_update_tracker_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_cordz_update_tracker_RES_DIRS_RELEASE )
set(abseil_absl_cordz_update_tracker_DEFINITIONS_RELEASE )
set(abseil_absl_cordz_update_tracker_OBJECTS_RELEASE )
set(abseil_absl_cordz_update_tracker_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_cordz_update_tracker_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_cordz_update_tracker_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_cordz_update_tracker_LIBS_RELEASE )
set(abseil_absl_cordz_update_tracker_SYSTEM_LIBS_RELEASE )
set(abseil_absl_cordz_update_tracker_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_cordz_update_tracker_FRAMEWORKS_RELEASE )
set(abseil_absl_cordz_update_tracker_DEPENDENCIES_RELEASE absl::config)
set(abseil_absl_cordz_update_tracker_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_cordz_update_tracker_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_cordz_update_tracker_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_cordz_update_tracker_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_cordz_update_tracker_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_cordz_update_tracker_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_cordz_update_tracker_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_cordz_update_tracker_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_cordz_update_tracker_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::random_internal_uniform_helper VARIABLES ############################################

set(abseil_absl_random_internal_uniform_helper_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_random_internal_uniform_helper_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_random_internal_uniform_helper_RES_DIRS_RELEASE )
set(abseil_absl_random_internal_uniform_helper_DEFINITIONS_RELEASE )
set(abseil_absl_random_internal_uniform_helper_OBJECTS_RELEASE )
set(abseil_absl_random_internal_uniform_helper_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_random_internal_uniform_helper_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_random_internal_uniform_helper_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_random_internal_uniform_helper_LIBS_RELEASE )
set(abseil_absl_random_internal_uniform_helper_SYSTEM_LIBS_RELEASE )
set(abseil_absl_random_internal_uniform_helper_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_random_internal_uniform_helper_FRAMEWORKS_RELEASE )
set(abseil_absl_random_internal_uniform_helper_DEPENDENCIES_RELEASE absl::config absl::random_internal_traits absl::type_traits)
set(abseil_absl_random_internal_uniform_helper_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_random_internal_uniform_helper_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_random_internal_uniform_helper_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_random_internal_uniform_helper_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_random_internal_uniform_helper_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_random_internal_uniform_helper_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_random_internal_uniform_helper_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_random_internal_uniform_helper_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_random_internal_uniform_helper_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::random_internal_randen_hwaes_impl VARIABLES ############################################

set(abseil_absl_random_internal_randen_hwaes_impl_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_random_internal_randen_hwaes_impl_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_random_internal_randen_hwaes_impl_RES_DIRS_RELEASE )
set(abseil_absl_random_internal_randen_hwaes_impl_DEFINITIONS_RELEASE )
set(abseil_absl_random_internal_randen_hwaes_impl_OBJECTS_RELEASE )
set(abseil_absl_random_internal_randen_hwaes_impl_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_random_internal_randen_hwaes_impl_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_random_internal_randen_hwaes_impl_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_random_internal_randen_hwaes_impl_LIBS_RELEASE absl_random_internal_randen_hwaes_impl)
set(abseil_absl_random_internal_randen_hwaes_impl_SYSTEM_LIBS_RELEASE )
set(abseil_absl_random_internal_randen_hwaes_impl_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_random_internal_randen_hwaes_impl_FRAMEWORKS_RELEASE )
set(abseil_absl_random_internal_randen_hwaes_impl_DEPENDENCIES_RELEASE absl::random_internal_platform absl::config)
set(abseil_absl_random_internal_randen_hwaes_impl_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_random_internal_randen_hwaes_impl_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_random_internal_randen_hwaes_impl_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_random_internal_randen_hwaes_impl_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_random_internal_randen_hwaes_impl_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_random_internal_randen_hwaes_impl_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_random_internal_randen_hwaes_impl_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_random_internal_randen_hwaes_impl_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_random_internal_randen_hwaes_impl_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::random_internal_randen_slow VARIABLES ############################################

set(abseil_absl_random_internal_randen_slow_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_random_internal_randen_slow_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_random_internal_randen_slow_RES_DIRS_RELEASE )
set(abseil_absl_random_internal_randen_slow_DEFINITIONS_RELEASE )
set(abseil_absl_random_internal_randen_slow_OBJECTS_RELEASE )
set(abseil_absl_random_internal_randen_slow_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_random_internal_randen_slow_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_random_internal_randen_slow_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_random_internal_randen_slow_LIBS_RELEASE absl_random_internal_randen_slow)
set(abseil_absl_random_internal_randen_slow_SYSTEM_LIBS_RELEASE )
set(abseil_absl_random_internal_randen_slow_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_random_internal_randen_slow_FRAMEWORKS_RELEASE )
set(abseil_absl_random_internal_randen_slow_DEPENDENCIES_RELEASE absl::random_internal_platform absl::config)
set(abseil_absl_random_internal_randen_slow_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_random_internal_randen_slow_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_random_internal_randen_slow_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_random_internal_randen_slow_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_random_internal_randen_slow_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_random_internal_randen_slow_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_random_internal_randen_slow_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_random_internal_randen_slow_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_random_internal_randen_slow_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::random_internal_platform VARIABLES ############################################

set(abseil_absl_random_internal_platform_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_random_internal_platform_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_random_internal_platform_RES_DIRS_RELEASE )
set(abseil_absl_random_internal_platform_DEFINITIONS_RELEASE )
set(abseil_absl_random_internal_platform_OBJECTS_RELEASE )
set(abseil_absl_random_internal_platform_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_random_internal_platform_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_random_internal_platform_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_random_internal_platform_LIBS_RELEASE absl_random_internal_platform)
set(abseil_absl_random_internal_platform_SYSTEM_LIBS_RELEASE )
set(abseil_absl_random_internal_platform_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_random_internal_platform_FRAMEWORKS_RELEASE )
set(abseil_absl_random_internal_platform_DEPENDENCIES_RELEASE absl::config)
set(abseil_absl_random_internal_platform_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_random_internal_platform_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_random_internal_platform_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_random_internal_platform_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_random_internal_platform_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_random_internal_platform_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_random_internal_platform_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_random_internal_platform_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_random_internal_platform_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::random_internal_pcg_engine VARIABLES ############################################

set(abseil_absl_random_internal_pcg_engine_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_random_internal_pcg_engine_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_random_internal_pcg_engine_RES_DIRS_RELEASE )
set(abseil_absl_random_internal_pcg_engine_DEFINITIONS_RELEASE )
set(abseil_absl_random_internal_pcg_engine_OBJECTS_RELEASE )
set(abseil_absl_random_internal_pcg_engine_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_random_internal_pcg_engine_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_random_internal_pcg_engine_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_random_internal_pcg_engine_LIBS_RELEASE )
set(abseil_absl_random_internal_pcg_engine_SYSTEM_LIBS_RELEASE )
set(abseil_absl_random_internal_pcg_engine_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_random_internal_pcg_engine_FRAMEWORKS_RELEASE )
set(abseil_absl_random_internal_pcg_engine_DEPENDENCIES_RELEASE absl::config absl::int128 absl::random_internal_fastmath absl::random_internal_iostream_state_saver absl::type_traits)
set(abseil_absl_random_internal_pcg_engine_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_random_internal_pcg_engine_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_random_internal_pcg_engine_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_random_internal_pcg_engine_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_random_internal_pcg_engine_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_random_internal_pcg_engine_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_random_internal_pcg_engine_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_random_internal_pcg_engine_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_random_internal_pcg_engine_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::random_internal_fastmath VARIABLES ############################################

set(abseil_absl_random_internal_fastmath_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_random_internal_fastmath_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_random_internal_fastmath_RES_DIRS_RELEASE )
set(abseil_absl_random_internal_fastmath_DEFINITIONS_RELEASE )
set(abseil_absl_random_internal_fastmath_OBJECTS_RELEASE )
set(abseil_absl_random_internal_fastmath_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_random_internal_fastmath_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_random_internal_fastmath_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_random_internal_fastmath_LIBS_RELEASE )
set(abseil_absl_random_internal_fastmath_SYSTEM_LIBS_RELEASE )
set(abseil_absl_random_internal_fastmath_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_random_internal_fastmath_FRAMEWORKS_RELEASE )
set(abseil_absl_random_internal_fastmath_DEPENDENCIES_RELEASE absl::bits)
set(abseil_absl_random_internal_fastmath_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_random_internal_fastmath_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_random_internal_fastmath_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_random_internal_fastmath_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_random_internal_fastmath_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_random_internal_fastmath_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_random_internal_fastmath_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_random_internal_fastmath_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_random_internal_fastmath_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::random_internal_wide_multiply VARIABLES ############################################

set(abseil_absl_random_internal_wide_multiply_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_random_internal_wide_multiply_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_random_internal_wide_multiply_RES_DIRS_RELEASE )
set(abseil_absl_random_internal_wide_multiply_DEFINITIONS_RELEASE )
set(abseil_absl_random_internal_wide_multiply_OBJECTS_RELEASE )
set(abseil_absl_random_internal_wide_multiply_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_random_internal_wide_multiply_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_random_internal_wide_multiply_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_random_internal_wide_multiply_LIBS_RELEASE )
set(abseil_absl_random_internal_wide_multiply_SYSTEM_LIBS_RELEASE )
set(abseil_absl_random_internal_wide_multiply_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_random_internal_wide_multiply_FRAMEWORKS_RELEASE )
set(abseil_absl_random_internal_wide_multiply_DEPENDENCIES_RELEASE absl::bits absl::config absl::int128)
set(abseil_absl_random_internal_wide_multiply_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_random_internal_wide_multiply_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_random_internal_wide_multiply_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_random_internal_wide_multiply_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_random_internal_wide_multiply_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_random_internal_wide_multiply_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_random_internal_wide_multiply_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_random_internal_wide_multiply_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_random_internal_wide_multiply_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::random_internal_iostream_state_saver VARIABLES ############################################

set(abseil_absl_random_internal_iostream_state_saver_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_random_internal_iostream_state_saver_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_random_internal_iostream_state_saver_RES_DIRS_RELEASE )
set(abseil_absl_random_internal_iostream_state_saver_DEFINITIONS_RELEASE )
set(abseil_absl_random_internal_iostream_state_saver_OBJECTS_RELEASE )
set(abseil_absl_random_internal_iostream_state_saver_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_random_internal_iostream_state_saver_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_random_internal_iostream_state_saver_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_random_internal_iostream_state_saver_LIBS_RELEASE )
set(abseil_absl_random_internal_iostream_state_saver_SYSTEM_LIBS_RELEASE )
set(abseil_absl_random_internal_iostream_state_saver_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_random_internal_iostream_state_saver_FRAMEWORKS_RELEASE )
set(abseil_absl_random_internal_iostream_state_saver_DEPENDENCIES_RELEASE absl::int128 absl::type_traits)
set(abseil_absl_random_internal_iostream_state_saver_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_random_internal_iostream_state_saver_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_random_internal_iostream_state_saver_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_random_internal_iostream_state_saver_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_random_internal_iostream_state_saver_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_random_internal_iostream_state_saver_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_random_internal_iostream_state_saver_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_random_internal_iostream_state_saver_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_random_internal_iostream_state_saver_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::random_internal_fast_uniform_bits VARIABLES ############################################

set(abseil_absl_random_internal_fast_uniform_bits_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_random_internal_fast_uniform_bits_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_random_internal_fast_uniform_bits_RES_DIRS_RELEASE )
set(abseil_absl_random_internal_fast_uniform_bits_DEFINITIONS_RELEASE )
set(abseil_absl_random_internal_fast_uniform_bits_OBJECTS_RELEASE )
set(abseil_absl_random_internal_fast_uniform_bits_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_random_internal_fast_uniform_bits_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_random_internal_fast_uniform_bits_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_random_internal_fast_uniform_bits_LIBS_RELEASE )
set(abseil_absl_random_internal_fast_uniform_bits_SYSTEM_LIBS_RELEASE )
set(abseil_absl_random_internal_fast_uniform_bits_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_random_internal_fast_uniform_bits_FRAMEWORKS_RELEASE )
set(abseil_absl_random_internal_fast_uniform_bits_DEPENDENCIES_RELEASE absl::config)
set(abseil_absl_random_internal_fast_uniform_bits_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_random_internal_fast_uniform_bits_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_random_internal_fast_uniform_bits_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_random_internal_fast_uniform_bits_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_random_internal_fast_uniform_bits_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_random_internal_fast_uniform_bits_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_random_internal_fast_uniform_bits_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_random_internal_fast_uniform_bits_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_random_internal_fast_uniform_bits_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::random_internal_traits VARIABLES ############################################

set(abseil_absl_random_internal_traits_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_random_internal_traits_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_random_internal_traits_RES_DIRS_RELEASE )
set(abseil_absl_random_internal_traits_DEFINITIONS_RELEASE )
set(abseil_absl_random_internal_traits_OBJECTS_RELEASE )
set(abseil_absl_random_internal_traits_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_random_internal_traits_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_random_internal_traits_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_random_internal_traits_LIBS_RELEASE )
set(abseil_absl_random_internal_traits_SYSTEM_LIBS_RELEASE )
set(abseil_absl_random_internal_traits_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_random_internal_traits_FRAMEWORKS_RELEASE )
set(abseil_absl_random_internal_traits_DEPENDENCIES_RELEASE absl::config)
set(abseil_absl_random_internal_traits_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_random_internal_traits_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_random_internal_traits_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_random_internal_traits_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_random_internal_traits_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_random_internal_traits_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_random_internal_traits_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_random_internal_traits_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_random_internal_traits_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::random_seed_gen_exception VARIABLES ############################################

set(abseil_absl_random_seed_gen_exception_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_random_seed_gen_exception_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_random_seed_gen_exception_RES_DIRS_RELEASE )
set(abseil_absl_random_seed_gen_exception_DEFINITIONS_RELEASE )
set(abseil_absl_random_seed_gen_exception_OBJECTS_RELEASE )
set(abseil_absl_random_seed_gen_exception_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_random_seed_gen_exception_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_random_seed_gen_exception_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_random_seed_gen_exception_LIBS_RELEASE absl_random_seed_gen_exception)
set(abseil_absl_random_seed_gen_exception_SYSTEM_LIBS_RELEASE )
set(abseil_absl_random_seed_gen_exception_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_random_seed_gen_exception_FRAMEWORKS_RELEASE )
set(abseil_absl_random_seed_gen_exception_DEPENDENCIES_RELEASE absl::config)
set(abseil_absl_random_seed_gen_exception_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_random_seed_gen_exception_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_random_seed_gen_exception_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_random_seed_gen_exception_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_random_seed_gen_exception_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_random_seed_gen_exception_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_random_seed_gen_exception_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_random_seed_gen_exception_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_random_seed_gen_exception_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::periodic_sampler VARIABLES ############################################

set(abseil_absl_periodic_sampler_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_periodic_sampler_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_periodic_sampler_RES_DIRS_RELEASE )
set(abseil_absl_periodic_sampler_DEFINITIONS_RELEASE )
set(abseil_absl_periodic_sampler_OBJECTS_RELEASE )
set(abseil_absl_periodic_sampler_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_periodic_sampler_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_periodic_sampler_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_periodic_sampler_LIBS_RELEASE absl_periodic_sampler)
set(abseil_absl_periodic_sampler_SYSTEM_LIBS_RELEASE )
set(abseil_absl_periodic_sampler_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_periodic_sampler_FRAMEWORKS_RELEASE )
set(abseil_absl_periodic_sampler_DEPENDENCIES_RELEASE absl::core_headers absl::exponential_biased)
set(abseil_absl_periodic_sampler_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_periodic_sampler_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_periodic_sampler_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_periodic_sampler_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_periodic_sampler_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_periodic_sampler_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_periodic_sampler_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_periodic_sampler_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_periodic_sampler_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::exponential_biased VARIABLES ############################################

set(abseil_absl_exponential_biased_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_exponential_biased_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_exponential_biased_RES_DIRS_RELEASE )
set(abseil_absl_exponential_biased_DEFINITIONS_RELEASE )
set(abseil_absl_exponential_biased_OBJECTS_RELEASE )
set(abseil_absl_exponential_biased_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_exponential_biased_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_exponential_biased_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_exponential_biased_LIBS_RELEASE absl_exponential_biased)
set(abseil_absl_exponential_biased_SYSTEM_LIBS_RELEASE )
set(abseil_absl_exponential_biased_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_exponential_biased_FRAMEWORKS_RELEASE )
set(abseil_absl_exponential_biased_DEPENDENCIES_RELEASE absl::config absl::core_headers)
set(abseil_absl_exponential_biased_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_exponential_biased_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_exponential_biased_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_exponential_biased_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_exponential_biased_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_exponential_biased_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_exponential_biased_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_exponential_biased_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_exponential_biased_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::numeric_representation VARIABLES ############################################

set(abseil_absl_numeric_representation_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_numeric_representation_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_numeric_representation_RES_DIRS_RELEASE )
set(abseil_absl_numeric_representation_DEFINITIONS_RELEASE )
set(abseil_absl_numeric_representation_OBJECTS_RELEASE )
set(abseil_absl_numeric_representation_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_numeric_representation_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_numeric_representation_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_numeric_representation_LIBS_RELEASE )
set(abseil_absl_numeric_representation_SYSTEM_LIBS_RELEASE )
set(abseil_absl_numeric_representation_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_numeric_representation_FRAMEWORKS_RELEASE )
set(abseil_absl_numeric_representation_DEPENDENCIES_RELEASE absl::config)
set(abseil_absl_numeric_representation_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_numeric_representation_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_numeric_representation_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_numeric_representation_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_numeric_representation_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_numeric_representation_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_numeric_representation_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_numeric_representation_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_numeric_representation_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::numeric VARIABLES ############################################

set(abseil_absl_numeric_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_numeric_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_numeric_RES_DIRS_RELEASE )
set(abseil_absl_numeric_DEFINITIONS_RELEASE )
set(abseil_absl_numeric_OBJECTS_RELEASE )
set(abseil_absl_numeric_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_numeric_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_numeric_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_numeric_LIBS_RELEASE )
set(abseil_absl_numeric_SYSTEM_LIBS_RELEASE )
set(abseil_absl_numeric_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_numeric_FRAMEWORKS_RELEASE )
set(abseil_absl_numeric_DEPENDENCIES_RELEASE absl::int128)
set(abseil_absl_numeric_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_numeric_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_numeric_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_numeric_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_numeric_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_numeric_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_numeric_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_numeric_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_numeric_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::int128 VARIABLES ############################################

set(abseil_absl_int128_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_int128_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_int128_RES_DIRS_RELEASE )
set(abseil_absl_int128_DEFINITIONS_RELEASE )
set(abseil_absl_int128_OBJECTS_RELEASE )
set(abseil_absl_int128_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_int128_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_int128_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_int128_LIBS_RELEASE absl_int128)
set(abseil_absl_int128_SYSTEM_LIBS_RELEASE )
set(abseil_absl_int128_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_int128_FRAMEWORKS_RELEASE )
set(abseil_absl_int128_DEPENDENCIES_RELEASE absl::config absl::core_headers absl::bits)
set(abseil_absl_int128_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_int128_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_int128_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_int128_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_int128_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_int128_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_int128_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_int128_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_int128_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::bits VARIABLES ############################################

set(abseil_absl_bits_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_bits_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_bits_RES_DIRS_RELEASE )
set(abseil_absl_bits_DEFINITIONS_RELEASE )
set(abseil_absl_bits_OBJECTS_RELEASE )
set(abseil_absl_bits_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_bits_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_bits_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_bits_LIBS_RELEASE )
set(abseil_absl_bits_SYSTEM_LIBS_RELEASE )
set(abseil_absl_bits_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_bits_FRAMEWORKS_RELEASE )
set(abseil_absl_bits_DEPENDENCIES_RELEASE absl::core_headers)
set(abseil_absl_bits_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_bits_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_bits_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_bits_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_bits_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_bits_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_bits_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_bits_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_bits_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::meta VARIABLES ############################################

set(abseil_absl_meta_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_meta_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_meta_RES_DIRS_RELEASE )
set(abseil_absl_meta_DEFINITIONS_RELEASE )
set(abseil_absl_meta_OBJECTS_RELEASE )
set(abseil_absl_meta_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_meta_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_meta_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_meta_LIBS_RELEASE )
set(abseil_absl_meta_SYSTEM_LIBS_RELEASE )
set(abseil_absl_meta_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_meta_FRAMEWORKS_RELEASE )
set(abseil_absl_meta_DEPENDENCIES_RELEASE absl::type_traits)
set(abseil_absl_meta_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_meta_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_meta_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_meta_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_meta_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_meta_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_meta_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_meta_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_meta_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::type_traits VARIABLES ############################################

set(abseil_absl_type_traits_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_type_traits_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_type_traits_RES_DIRS_RELEASE )
set(abseil_absl_type_traits_DEFINITIONS_RELEASE )
set(abseil_absl_type_traits_OBJECTS_RELEASE )
set(abseil_absl_type_traits_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_type_traits_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_type_traits_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_type_traits_LIBS_RELEASE )
set(abseil_absl_type_traits_SYSTEM_LIBS_RELEASE )
set(abseil_absl_type_traits_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_type_traits_FRAMEWORKS_RELEASE )
set(abseil_absl_type_traits_DEPENDENCIES_RELEASE absl::config)
set(abseil_absl_type_traits_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_type_traits_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_type_traits_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_type_traits_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_type_traits_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_type_traits_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_type_traits_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_type_traits_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_type_traits_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::flags_commandlineflag_internal VARIABLES ############################################

set(abseil_absl_flags_commandlineflag_internal_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_flags_commandlineflag_internal_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_flags_commandlineflag_internal_RES_DIRS_RELEASE )
set(abseil_absl_flags_commandlineflag_internal_DEFINITIONS_RELEASE )
set(abseil_absl_flags_commandlineflag_internal_OBJECTS_RELEASE )
set(abseil_absl_flags_commandlineflag_internal_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_flags_commandlineflag_internal_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_flags_commandlineflag_internal_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_flags_commandlineflag_internal_LIBS_RELEASE absl_flags_commandlineflag_internal)
set(abseil_absl_flags_commandlineflag_internal_SYSTEM_LIBS_RELEASE )
set(abseil_absl_flags_commandlineflag_internal_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_flags_commandlineflag_internal_FRAMEWORKS_RELEASE )
set(abseil_absl_flags_commandlineflag_internal_DEPENDENCIES_RELEASE absl::config absl::dynamic_annotations absl::fast_type_id)
set(abseil_absl_flags_commandlineflag_internal_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_flags_commandlineflag_internal_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_flags_commandlineflag_internal_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_flags_commandlineflag_internal_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_flags_commandlineflag_internal_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_flags_commandlineflag_internal_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_flags_commandlineflag_internal_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_flags_commandlineflag_internal_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_flags_commandlineflag_internal_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::leak_check VARIABLES ############################################

set(abseil_absl_leak_check_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_leak_check_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_leak_check_RES_DIRS_RELEASE )
set(abseil_absl_leak_check_DEFINITIONS_RELEASE )
set(abseil_absl_leak_check_OBJECTS_RELEASE )
set(abseil_absl_leak_check_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_leak_check_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_leak_check_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_leak_check_LIBS_RELEASE absl_leak_check)
set(abseil_absl_leak_check_SYSTEM_LIBS_RELEASE )
set(abseil_absl_leak_check_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_leak_check_FRAMEWORKS_RELEASE )
set(abseil_absl_leak_check_DEPENDENCIES_RELEASE absl::config absl::core_headers)
set(abseil_absl_leak_check_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_leak_check_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_leak_check_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_leak_check_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_leak_check_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_leak_check_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_leak_check_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_leak_check_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_leak_check_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::node_slot_policy VARIABLES ############################################

set(abseil_absl_node_slot_policy_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_node_slot_policy_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_node_slot_policy_RES_DIRS_RELEASE )
set(abseil_absl_node_slot_policy_DEFINITIONS_RELEASE )
set(abseil_absl_node_slot_policy_OBJECTS_RELEASE )
set(abseil_absl_node_slot_policy_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_node_slot_policy_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_node_slot_policy_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_node_slot_policy_LIBS_RELEASE )
set(abseil_absl_node_slot_policy_SYSTEM_LIBS_RELEASE )
set(abseil_absl_node_slot_policy_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_node_slot_policy_FRAMEWORKS_RELEASE )
set(abseil_absl_node_slot_policy_DEPENDENCIES_RELEASE absl::config)
set(abseil_absl_node_slot_policy_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_node_slot_policy_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_node_slot_policy_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_node_slot_policy_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_node_slot_policy_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_node_slot_policy_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_node_slot_policy_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_node_slot_policy_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_node_slot_policy_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::hashtable_debug_hooks VARIABLES ############################################

set(abseil_absl_hashtable_debug_hooks_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_hashtable_debug_hooks_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_hashtable_debug_hooks_RES_DIRS_RELEASE )
set(abseil_absl_hashtable_debug_hooks_DEFINITIONS_RELEASE )
set(abseil_absl_hashtable_debug_hooks_OBJECTS_RELEASE )
set(abseil_absl_hashtable_debug_hooks_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_hashtable_debug_hooks_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_hashtable_debug_hooks_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_hashtable_debug_hooks_LIBS_RELEASE )
set(abseil_absl_hashtable_debug_hooks_SYSTEM_LIBS_RELEASE )
set(abseil_absl_hashtable_debug_hooks_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_hashtable_debug_hooks_FRAMEWORKS_RELEASE )
set(abseil_absl_hashtable_debug_hooks_DEPENDENCIES_RELEASE absl::config)
set(abseil_absl_hashtable_debug_hooks_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_hashtable_debug_hooks_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_hashtable_debug_hooks_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_hashtable_debug_hooks_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_hashtable_debug_hooks_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_hashtable_debug_hooks_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_hashtable_debug_hooks_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_hashtable_debug_hooks_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_hashtable_debug_hooks_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::counting_allocator VARIABLES ############################################

set(abseil_absl_counting_allocator_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_counting_allocator_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_counting_allocator_RES_DIRS_RELEASE )
set(abseil_absl_counting_allocator_DEFINITIONS_RELEASE )
set(abseil_absl_counting_allocator_OBJECTS_RELEASE )
set(abseil_absl_counting_allocator_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_counting_allocator_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_counting_allocator_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_counting_allocator_LIBS_RELEASE )
set(abseil_absl_counting_allocator_SYSTEM_LIBS_RELEASE )
set(abseil_absl_counting_allocator_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_counting_allocator_FRAMEWORKS_RELEASE )
set(abseil_absl_counting_allocator_DEPENDENCIES_RELEASE absl::config)
set(abseil_absl_counting_allocator_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_counting_allocator_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_counting_allocator_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_counting_allocator_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_counting_allocator_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_counting_allocator_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_counting_allocator_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_counting_allocator_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_counting_allocator_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::algorithm VARIABLES ############################################

set(abseil_absl_algorithm_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_algorithm_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_algorithm_RES_DIRS_RELEASE )
set(abseil_absl_algorithm_DEFINITIONS_RELEASE )
set(abseil_absl_algorithm_OBJECTS_RELEASE )
set(abseil_absl_algorithm_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_algorithm_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_algorithm_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_algorithm_LIBS_RELEASE )
set(abseil_absl_algorithm_SYSTEM_LIBS_RELEASE )
set(abseil_absl_algorithm_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_algorithm_FRAMEWORKS_RELEASE )
set(abseil_absl_algorithm_DEPENDENCIES_RELEASE absl::config)
set(abseil_absl_algorithm_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_algorithm_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_algorithm_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_algorithm_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_algorithm_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_algorithm_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_algorithm_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_algorithm_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_algorithm_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::prefetch VARIABLES ############################################

set(abseil_absl_prefetch_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_prefetch_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_prefetch_RES_DIRS_RELEASE )
set(abseil_absl_prefetch_DEFINITIONS_RELEASE )
set(abseil_absl_prefetch_OBJECTS_RELEASE )
set(abseil_absl_prefetch_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_prefetch_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_prefetch_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_prefetch_LIBS_RELEASE )
set(abseil_absl_prefetch_SYSTEM_LIBS_RELEASE )
set(abseil_absl_prefetch_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_prefetch_FRAMEWORKS_RELEASE )
set(abseil_absl_prefetch_DEPENDENCIES_RELEASE absl::config)
set(abseil_absl_prefetch_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_prefetch_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_prefetch_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_prefetch_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_prefetch_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_prefetch_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_prefetch_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_prefetch_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_prefetch_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::fast_type_id VARIABLES ############################################

set(abseil_absl_fast_type_id_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_fast_type_id_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_fast_type_id_RES_DIRS_RELEASE )
set(abseil_absl_fast_type_id_DEFINITIONS_RELEASE )
set(abseil_absl_fast_type_id_OBJECTS_RELEASE )
set(abseil_absl_fast_type_id_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_fast_type_id_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_fast_type_id_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_fast_type_id_LIBS_RELEASE )
set(abseil_absl_fast_type_id_SYSTEM_LIBS_RELEASE )
set(abseil_absl_fast_type_id_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_fast_type_id_FRAMEWORKS_RELEASE )
set(abseil_absl_fast_type_id_DEPENDENCIES_RELEASE absl::config)
set(abseil_absl_fast_type_id_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_fast_type_id_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_fast_type_id_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_fast_type_id_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_fast_type_id_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_fast_type_id_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_fast_type_id_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_fast_type_id_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_fast_type_id_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::pretty_function VARIABLES ############################################

set(abseil_absl_pretty_function_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_pretty_function_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_pretty_function_RES_DIRS_RELEASE )
set(abseil_absl_pretty_function_DEFINITIONS_RELEASE )
set(abseil_absl_pretty_function_OBJECTS_RELEASE )
set(abseil_absl_pretty_function_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_pretty_function_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_pretty_function_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_pretty_function_LIBS_RELEASE )
set(abseil_absl_pretty_function_SYSTEM_LIBS_RELEASE )
set(abseil_absl_pretty_function_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_pretty_function_FRAMEWORKS_RELEASE )
set(abseil_absl_pretty_function_DEPENDENCIES_RELEASE )
set(abseil_absl_pretty_function_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_pretty_function_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_pretty_function_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_pretty_function_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_pretty_function_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_pretty_function_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_pretty_function_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_pretty_function_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_pretty_function_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::core_headers VARIABLES ############################################

set(abseil_absl_core_headers_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_core_headers_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_core_headers_RES_DIRS_RELEASE )
set(abseil_absl_core_headers_DEFINITIONS_RELEASE )
set(abseil_absl_core_headers_OBJECTS_RELEASE )
set(abseil_absl_core_headers_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_core_headers_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_core_headers_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_core_headers_LIBS_RELEASE )
set(abseil_absl_core_headers_SYSTEM_LIBS_RELEASE )
set(abseil_absl_core_headers_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_core_headers_FRAMEWORKS_RELEASE )
set(abseil_absl_core_headers_DEPENDENCIES_RELEASE absl::config)
set(abseil_absl_core_headers_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_core_headers_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_core_headers_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_core_headers_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_core_headers_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_core_headers_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_core_headers_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_core_headers_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_core_headers_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::dynamic_annotations VARIABLES ############################################

set(abseil_absl_dynamic_annotations_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_dynamic_annotations_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_dynamic_annotations_RES_DIRS_RELEASE )
set(abseil_absl_dynamic_annotations_DEFINITIONS_RELEASE )
set(abseil_absl_dynamic_annotations_OBJECTS_RELEASE )
set(abseil_absl_dynamic_annotations_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_dynamic_annotations_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_dynamic_annotations_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_dynamic_annotations_LIBS_RELEASE )
set(abseil_absl_dynamic_annotations_SYSTEM_LIBS_RELEASE )
set(abseil_absl_dynamic_annotations_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_dynamic_annotations_FRAMEWORKS_RELEASE )
set(abseil_absl_dynamic_annotations_DEPENDENCIES_RELEASE absl::config)
set(abseil_absl_dynamic_annotations_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_dynamic_annotations_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_dynamic_annotations_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_dynamic_annotations_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_dynamic_annotations_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_dynamic_annotations_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_dynamic_annotations_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_dynamic_annotations_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_dynamic_annotations_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT absl::config VARIABLES ############################################

set(abseil_absl_config_INCLUDE_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/include")
set(abseil_absl_config_LIB_DIRS_RELEASE "${abseil_PACKAGE_FOLDER_RELEASE}/lib")
set(abseil_absl_config_RES_DIRS_RELEASE )
set(abseil_absl_config_DEFINITIONS_RELEASE )
set(abseil_absl_config_OBJECTS_RELEASE )
set(abseil_absl_config_COMPILE_DEFINITIONS_RELEASE )
set(abseil_absl_config_COMPILE_OPTIONS_C_RELEASE "")
set(abseil_absl_config_COMPILE_OPTIONS_CXX_RELEASE "")
set(abseil_absl_config_LIBS_RELEASE )
set(abseil_absl_config_SYSTEM_LIBS_RELEASE )
set(abseil_absl_config_FRAMEWORK_DIRS_RELEASE )
set(abseil_absl_config_FRAMEWORKS_RELEASE )
set(abseil_absl_config_DEPENDENCIES_RELEASE )
set(abseil_absl_config_SHARED_LINK_FLAGS_RELEASE )
set(abseil_absl_config_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(abseil_absl_config_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${abseil_absl_config_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${abseil_absl_config_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${abseil_absl_config_EXE_LINK_FLAGS_RELEASE}>
)
set(abseil_absl_config_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${abseil_absl_config_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${abseil_absl_config_COMPILE_OPTIONS_C_RELEASE}>")