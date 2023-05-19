# Avoid multiple calls to find_package to append duplicated properties to the targets
include_guard()########### VARIABLES #######################################################################
#############################################################################################
set(abseil_FRAMEWORKS_FOUND_RELEASE "") # Will be filled later
conan_find_apple_frameworks(abseil_FRAMEWORKS_FOUND_RELEASE "${abseil_FRAMEWORKS_RELEASE}" "${abseil_FRAMEWORK_DIRS_RELEASE}")

set(abseil_LIBRARIES_TARGETS "") # Will be filled later


######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
if(NOT TARGET abseil_DEPS_TARGET)
    add_library(abseil_DEPS_TARGET INTERFACE IMPORTED)
endif()

set_property(TARGET abseil_DEPS_TARGET
             PROPERTY INTERFACE_LINK_LIBRARIES
             $<$<CONFIG:Release>:${abseil_FRAMEWORKS_FOUND_RELEASE}>
             $<$<CONFIG:Release>:${abseil_SYSTEM_LIBS_RELEASE}>
             $<$<CONFIG:Release>:absl::config;absl::core_headers;absl::config;absl::core_headers;absl::atomic_hook;absl::config;absl::core_headers;absl::errno_saver;absl::log_severity;absl::base_internal;absl::core_headers;absl::errno_saver;absl::config;absl::config;absl::base;absl::base_internal;absl::config;absl::core_headers;absl::dynamic_annotations;absl::raw_logging_internal;absl::config;absl::type_traits;absl::atomic_hook;absl::base_internal;absl::config;absl::core_headers;absl::dynamic_annotations;absl::log_severity;absl::raw_logging_internal;absl::spinlock_wait;absl::type_traits;absl::config;absl::raw_logging_internal;absl::base;absl::config;absl::core_headers;absl::config;absl::raw_logging_internal;absl::config;absl::core_headers;absl::errno_saver;absl::config;absl::config;absl::config;absl::algorithm;absl::core_headers;absl::meta;absl::base_internal;absl::core_headers;absl::utility;absl::cleanup_internal;absl::config;absl::core_headers;absl::container_common;absl::compare;absl::compressed_tuple;absl::container_memory;absl::cord;absl::core_headers;absl::layout;absl::memory;absl::raw_logging_internal;absl::strings;absl::throw_delegate;absl::type_traits;absl::utility;absl::utility;absl::compressed_tuple;absl::algorithm;absl::config;absl::core_headers;absl::dynamic_annotations;absl::throw_delegate;absl::memory;absl::compressed_tuple;absl::core_headers;absl::memory;absl::span;absl::type_traits;absl::algorithm;absl::core_headers;absl::inlined_vector_internal;absl::throw_delegate;absl::memory;absl::config;absl::container_memory;absl::core_headers;absl::hash_function_defaults;absl::raw_hash_map;absl::algorithm_container;absl::memory;absl::container_memory;absl::hash_function_defaults;absl::raw_hash_set;absl::algorithm_container;absl::core_headers;absl::memory;absl::container_memory;absl::core_headers;absl::hash_function_defaults;absl::node_slot_policy;absl::raw_hash_map;absl::algorithm_container;absl::memory;absl::core_headers;absl::hash_function_defaults;absl::node_slot_policy;absl::raw_hash_set;absl::algorithm_container;absl::memory;absl::config;absl::memory;absl::type_traits;absl::utility;absl::config;absl::cord;absl::hash;absl::strings;absl::meta;absl::base;absl::config;absl::exponential_biased;absl::sample_recorder;absl::synchronization;absl::hashtable_debug_hooks;absl::config;absl::config;absl::container_memory;absl::raw_hash_set;absl::throw_delegate;absl::type_traits;absl::bits;absl::compressed_tuple;absl::config;absl::container_common;absl::container_memory;absl::core_headers;absl::endian;absl::hash_policy_traits;absl::hashtable_debug_hooks;absl::memory;absl::meta;absl::optional;absl::prefetch;absl::utility;absl::hashtablez_sampler;absl::config;absl::core_headers;absl::meta;absl::strings;absl::span;absl::utility;absl::debugging_internal;absl::config;absl::core_headers;absl::debugging_internal;absl::demangle_internal;absl::base;absl::config;absl::core_headers;absl::dynamic_annotations;absl::malloc_internal;absl::raw_logging_internal;absl::strings;absl::stacktrace;absl::symbolize;absl::config;absl::core_headers;absl::raw_logging_internal;absl::examine_stack;absl::stacktrace;absl::base;absl::config;absl::core_headers;absl::raw_logging_internal;absl::core_headers;absl::config;absl::dynamic_annotations;absl::errno_saver;absl::raw_logging_internal;absl::base;absl::core_headers;absl::config;absl::core_headers;absl::stacktrace;absl::leak_check;absl::config;absl::strings;absl::config;absl::core_headers;absl::flags_path_util;absl::strings;absl::synchronization;absl::config;absl::flags_path_util;absl::flags_program_name;absl::core_headers;absl::strings;absl::synchronization;absl::config;absl::core_headers;absl::log_severity;absl::optional;absl::strings;absl::str_format;absl::config;absl::dynamic_annotations;absl::fast_type_id;absl::config;absl::fast_type_id;absl::flags_commandlineflag_internal;absl::optional;absl::strings;absl::config;absl::flags_commandlineflag;absl::flags_commandlineflag_internal;absl::strings;absl::config;absl::flags_commandlineflag;absl::flags_private_handle_accessor;absl::flags_config;absl::strings;absl::synchronization;absl::flat_hash_map;absl::base;absl::config;absl::flags_commandlineflag;absl::flags_commandlineflag_internal;absl::flags_config;absl::flags_marshalling;absl::synchronization;absl::meta;absl::utility;absl::config;absl::flags_commandlineflag;absl::flags_config;absl::flags_internal;absl::flags_reflection;absl::base;absl::core_headers;absl::strings;absl::config;absl::flags_config;absl::flags;absl::flags_commandlineflag;absl::flags_internal;absl::flags_path_util;absl::flags_private_handle_accessor;absl::flags_program_name;absl::flags_reflection;absl::flat_hash_map;absl::strings;absl::synchronization;absl::config;absl::core_headers;absl::flags_usage_internal;absl::strings;absl::synchronization;absl::config;absl::core_headers;absl::flags_config;absl::flags;absl::flags_commandlineflag;absl::flags_commandlineflag_internal;absl::flags_internal;absl::flags_private_handle_accessor;absl::flags_program_name;absl::flags_reflection;absl::flags_usage;absl::strings;absl::synchronization;absl::base_internal;absl::config;absl::core_headers;absl::type_traits;absl::utility;absl::base_internal;absl::compressed_tuple;absl::base_internal;absl::core_headers;absl::meta;absl::city;absl::config;absl::core_headers;absl::endian;absl::fixed_array;absl::function_ref;absl::meta;absl::int128;absl::strings;absl::optional;absl::variant;absl::utility;absl::low_level_hash;absl::config;absl::core_headers;absl::endian;absl::bits;absl::config;absl::endian;absl::int128;absl::core_headers;absl::meta;absl::config;absl::type_traits;absl::core_headers;absl::config;absl::core_headers;absl::bits;absl::int128;absl::config;absl::base;absl::synchronization;absl::config;absl::core_headers;absl::core_headers;absl::exponential_biased;absl::random_distributions;absl::random_internal_nonsecure_base;absl::random_internal_pcg_engine;absl::random_internal_pool_urbg;absl::random_internal_randen_engine;absl::random_seed_sequences;absl::core_headers;absl::random_internal_distribution_caller;absl::random_internal_fast_uniform_bits;absl::type_traits;absl::fast_type_id;absl::optional;absl::base_internal;absl::config;absl::core_headers;absl::random_internal_generate_real;absl::random_internal_distribution_caller;absl::random_internal_fast_uniform_bits;absl::random_internal_fastmath;absl::random_internal_iostream_state_saver;absl::random_internal_traits;absl::random_internal_uniform_helper;absl::random_internal_wide_multiply;absl::strings;absl::type_traits;absl::config;absl::config;absl::inlined_vector;absl::random_internal_pool_urbg;absl::random_internal_salted_seed_seq;absl::random_internal_seed_material;absl::random_seed_gen_exception;absl::span;absl::config;absl::config;absl::utility;absl::fast_type_id;absl::config;absl::core_headers;absl::optional;absl::random_internal_fast_uniform_bits;absl::raw_logging_internal;absl::span;absl::strings;absl::base;absl::config;absl::core_headers;absl::endian;absl::random_internal_randen;absl::random_internal_seed_material;absl::random_internal_traits;absl::random_seed_gen_exception;absl::raw_logging_internal;absl::span;absl::inlined_vector;absl::optional;absl::span;absl::random_internal_seed_material;absl::type_traits;absl::int128;absl::type_traits;absl::bits;absl::random_internal_fastmath;absl::random_internal_traits;absl::type_traits;absl::bits;absl::config;absl::int128;absl::bits;absl::core_headers;absl::inlined_vector;absl::random_internal_pool_urbg;absl::random_internal_salted_seed_seq;absl::random_internal_seed_material;absl::span;absl::type_traits;absl::config;absl::int128;absl::random_internal_fastmath;absl::random_internal_iostream_state_saver;absl::type_traits;absl::endian;absl::random_internal_iostream_state_saver;absl::random_internal_randen;absl::raw_logging_internal;absl::type_traits;absl::config;absl::random_internal_platform;absl::random_internal_randen_hwaes;absl::random_internal_randen_slow;absl::random_internal_platform;absl::config;absl::random_internal_platform;absl::random_internal_randen_hwaes_impl;absl::config;absl::random_internal_platform;absl::config;absl::config;absl::core_headers;absl::raw_logging_internal;absl::strings;absl::str_format;absl::span;absl::config;absl::random_internal_traits;absl::type_traits;absl::atomic_hook;absl::config;absl::cord;absl::core_headers;absl::function_ref;absl::inlined_vector;absl::optional;absl::raw_logging_internal;absl::stacktrace;absl::str_format;absl::strerror;absl::strings;absl::symbolize;absl::base;absl::status;absl::core_headers;absl::raw_logging_internal;absl::type_traits;absl::strings;absl::utility;absl::variant;absl::strings_internal;absl::base;absl::bits;absl::config;absl::core_headers;absl::endian;absl::int128;absl::memory;absl::raw_logging_internal;absl::throw_delegate;absl::type_traits;absl::config;absl::core_headers;absl::endian;absl::raw_logging_internal;absl::type_traits;absl::str_format_internal;absl::bits;absl::strings;absl::config;absl::core_headers;absl::numeric_representation;absl::type_traits;absl::utility;absl::int128;absl::span;absl::base_internal;absl::compressed_tuple;absl::config;absl::core_headers;absl::endian;absl::inlined_vector;absl::layout;absl::raw_logging_internal;absl::strings;absl::throw_delegate;absl::type_traits;absl::config;absl::config;absl::core_headers;absl::exponential_biased;absl::raw_logging_internal;absl::config;absl::core_headers;absl::cordz_update_tracker;absl::synchronization;absl::base;absl::config;absl::raw_logging_internal;absl::synchronization;absl::base;absl::config;absl::cord_internal;absl::cordz_functions;absl::cordz_handle;absl::cordz_statistics;absl::cordz_update_tracker;absl::core_headers;absl::inlined_vector;absl::span;absl::raw_logging_internal;absl::stacktrace;absl::synchronization;absl::config;absl::cordz_handle;absl::cordz_info;absl::config;absl::cord_internal;absl::cordz_info;absl::cordz_update_tracker;absl::core_headers;absl::base;absl::config;absl::cord_internal;absl::cordz_functions;absl::cordz_info;absl::cordz_update_scope;absl::cordz_update_tracker;absl::core_headers;absl::endian;absl::fixed_array;absl::function_ref;absl::inlined_vector;absl::optional;absl::raw_logging_internal;absl::span;absl::strings;absl::type_traits;absl::base;absl::base_internal;absl::config;absl::core_headers;absl::malloc_internal;absl::raw_logging_internal;absl::core_headers;absl::raw_logging_internal;absl::time;absl::graphcycles_internal;absl::kernel_timeout_internal;absl::atomic_hook;absl::base;absl::base_internal;absl::config;absl::core_headers;absl::dynamic_annotations;absl::malloc_internal;absl::raw_logging_internal;absl::stacktrace;absl::symbolize;absl::time;absl::base;absl::civil_time;absl::core_headers;absl::int128;absl::raw_logging_internal;absl::strings;absl::time_zone;absl::bad_any_cast;absl::config;absl::core_headers;absl::fast_type_id;absl::type_traits;absl::utility;absl::bad_any_cast_impl;absl::config;absl::config;absl::raw_logging_internal;absl::algorithm;absl::core_headers;absl::throw_delegate;absl::type_traits;absl::bad_optional_access;absl::base_internal;absl::config;absl::core_headers;absl::memory;absl::type_traits;absl::utility;absl::config;absl::raw_logging_internal;absl::config;absl::raw_logging_internal;absl::bad_variant_access;absl::base_internal;absl::config;absl::core_headers;absl::type_traits;absl::utility;absl::core_headers;absl::type_traits;absl::base_internal;absl::config;absl::type_traits>
             APPEND)

####### Find the libraries declared in cpp_info.libs, create an IMPORTED target for each one and link the
####### abseil_DEPS_TARGET to all of them
conan_package_library_targets("${abseil_LIBS_RELEASE}"    # libraries
                              "${abseil_LIB_DIRS_RELEASE}" # package_libdir
                              abseil_DEPS_TARGET
                              abseil_LIBRARIES_TARGETS  # out_libraries_targets
                              "_RELEASE"
                              "abseil")    # package_name

# FIXME: What is the result of this for multi-config? All configs adding themselves to path?
set(CMAKE_MODULE_PATH ${abseil_BUILD_DIRS_RELEASE} ${CMAKE_MODULE_PATH})

########## COMPONENTS TARGET PROPERTIES Release ########################################

    ########## COMPONENT absl::flags_parse #############

        set(abseil_absl_flags_parse_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_flags_parse_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_flags_parse_FRAMEWORKS_RELEASE}" "${abseil_absl_flags_parse_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_flags_parse_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_flags_parse_DEPS_TARGET)
            add_library(abseil_absl_flags_parse_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_flags_parse_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_flags_parse_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_flags_parse_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_flags_parse_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_flags_parse_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_flags_parse_LIBS_RELEASE}"
                                      "${abseil_absl_flags_parse_LIB_DIRS_RELEASE}"
                                      abseil_absl_flags_parse_DEPS_TARGET
                                      abseil_absl_flags_parse_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_flags_parse")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::flags_parse
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_flags_parse_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_flags_parse_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_flags_parse_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::flags_parse
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_flags_parse_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::flags_parse PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_flags_parse_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::flags_parse PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_flags_parse_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::flags_parse PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_flags_parse_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::flags_parse PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_flags_parse_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::flags_usage #############

        set(abseil_absl_flags_usage_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_flags_usage_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_flags_usage_FRAMEWORKS_RELEASE}" "${abseil_absl_flags_usage_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_flags_usage_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_flags_usage_DEPS_TARGET)
            add_library(abseil_absl_flags_usage_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_flags_usage_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_flags_usage_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_flags_usage_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_flags_usage_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_flags_usage_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_flags_usage_LIBS_RELEASE}"
                                      "${abseil_absl_flags_usage_LIB_DIRS_RELEASE}"
                                      abseil_absl_flags_usage_DEPS_TARGET
                                      abseil_absl_flags_usage_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_flags_usage")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::flags_usage
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_flags_usage_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_flags_usage_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_flags_usage_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::flags_usage
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_flags_usage_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::flags_usage PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_flags_usage_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::flags_usage PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_flags_usage_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::flags_usage PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_flags_usage_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::flags_usage PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_flags_usage_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::flags_usage_internal #############

        set(abseil_absl_flags_usage_internal_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_flags_usage_internal_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_flags_usage_internal_FRAMEWORKS_RELEASE}" "${abseil_absl_flags_usage_internal_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_flags_usage_internal_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_flags_usage_internal_DEPS_TARGET)
            add_library(abseil_absl_flags_usage_internal_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_flags_usage_internal_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_flags_usage_internal_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_flags_usage_internal_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_flags_usage_internal_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_flags_usage_internal_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_flags_usage_internal_LIBS_RELEASE}"
                                      "${abseil_absl_flags_usage_internal_LIB_DIRS_RELEASE}"
                                      abseil_absl_flags_usage_internal_DEPS_TARGET
                                      abseil_absl_flags_usage_internal_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_flags_usage_internal")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::flags_usage_internal
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_flags_usage_internal_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_flags_usage_internal_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_flags_usage_internal_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::flags_usage_internal
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_flags_usage_internal_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::flags_usage_internal PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_flags_usage_internal_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::flags_usage_internal PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_flags_usage_internal_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::flags_usage_internal PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_flags_usage_internal_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::flags_usage_internal PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_flags_usage_internal_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::flags #############

        set(abseil_absl_flags_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_flags_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_flags_FRAMEWORKS_RELEASE}" "${abseil_absl_flags_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_flags_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_flags_DEPS_TARGET)
            add_library(abseil_absl_flags_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_flags_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_flags_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_flags_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_flags_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_flags_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_flags_LIBS_RELEASE}"
                                      "${abseil_absl_flags_LIB_DIRS_RELEASE}"
                                      abseil_absl_flags_DEPS_TARGET
                                      abseil_absl_flags_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_flags")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::flags
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_flags_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_flags_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_flags_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::flags
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_flags_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::flags PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_flags_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::flags PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_flags_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::flags PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_flags_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::flags PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_flags_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::flags_reflection #############

        set(abseil_absl_flags_reflection_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_flags_reflection_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_flags_reflection_FRAMEWORKS_RELEASE}" "${abseil_absl_flags_reflection_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_flags_reflection_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_flags_reflection_DEPS_TARGET)
            add_library(abseil_absl_flags_reflection_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_flags_reflection_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_flags_reflection_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_flags_reflection_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_flags_reflection_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_flags_reflection_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_flags_reflection_LIBS_RELEASE}"
                                      "${abseil_absl_flags_reflection_LIB_DIRS_RELEASE}"
                                      abseil_absl_flags_reflection_DEPS_TARGET
                                      abseil_absl_flags_reflection_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_flags_reflection")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::flags_reflection
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_flags_reflection_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_flags_reflection_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_flags_reflection_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::flags_reflection
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_flags_reflection_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::flags_reflection PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_flags_reflection_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::flags_reflection PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_flags_reflection_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::flags_reflection PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_flags_reflection_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::flags_reflection PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_flags_reflection_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::node_hash_map #############

        set(abseil_absl_node_hash_map_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_node_hash_map_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_node_hash_map_FRAMEWORKS_RELEASE}" "${abseil_absl_node_hash_map_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_node_hash_map_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_node_hash_map_DEPS_TARGET)
            add_library(abseil_absl_node_hash_map_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_node_hash_map_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_node_hash_map_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_node_hash_map_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_node_hash_map_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_node_hash_map_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_node_hash_map_LIBS_RELEASE}"
                                      "${abseil_absl_node_hash_map_LIB_DIRS_RELEASE}"
                                      abseil_absl_node_hash_map_DEPS_TARGET
                                      abseil_absl_node_hash_map_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_node_hash_map")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::node_hash_map
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_node_hash_map_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_node_hash_map_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_node_hash_map_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::node_hash_map
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_node_hash_map_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::node_hash_map PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_node_hash_map_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::node_hash_map PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_node_hash_map_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::node_hash_map PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_node_hash_map_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::node_hash_map PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_node_hash_map_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::flat_hash_map #############

        set(abseil_absl_flat_hash_map_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_flat_hash_map_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_flat_hash_map_FRAMEWORKS_RELEASE}" "${abseil_absl_flat_hash_map_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_flat_hash_map_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_flat_hash_map_DEPS_TARGET)
            add_library(abseil_absl_flat_hash_map_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_flat_hash_map_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_flat_hash_map_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_flat_hash_map_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_flat_hash_map_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_flat_hash_map_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_flat_hash_map_LIBS_RELEASE}"
                                      "${abseil_absl_flat_hash_map_LIB_DIRS_RELEASE}"
                                      abseil_absl_flat_hash_map_DEPS_TARGET
                                      abseil_absl_flat_hash_map_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_flat_hash_map")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::flat_hash_map
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_flat_hash_map_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_flat_hash_map_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_flat_hash_map_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::flat_hash_map
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_flat_hash_map_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::flat_hash_map PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_flat_hash_map_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::flat_hash_map PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_flat_hash_map_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::flat_hash_map PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_flat_hash_map_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::flat_hash_map PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_flat_hash_map_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::raw_hash_map #############

        set(abseil_absl_raw_hash_map_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_raw_hash_map_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_raw_hash_map_FRAMEWORKS_RELEASE}" "${abseil_absl_raw_hash_map_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_raw_hash_map_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_raw_hash_map_DEPS_TARGET)
            add_library(abseil_absl_raw_hash_map_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_raw_hash_map_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_raw_hash_map_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_raw_hash_map_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_raw_hash_map_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_raw_hash_map_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_raw_hash_map_LIBS_RELEASE}"
                                      "${abseil_absl_raw_hash_map_LIB_DIRS_RELEASE}"
                                      abseil_absl_raw_hash_map_DEPS_TARGET
                                      abseil_absl_raw_hash_map_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_raw_hash_map")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::raw_hash_map
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_raw_hash_map_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_raw_hash_map_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_raw_hash_map_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::raw_hash_map
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_raw_hash_map_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::raw_hash_map PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_raw_hash_map_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::raw_hash_map PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_raw_hash_map_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::raw_hash_map PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_raw_hash_map_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::raw_hash_map PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_raw_hash_map_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::node_hash_set #############

        set(abseil_absl_node_hash_set_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_node_hash_set_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_node_hash_set_FRAMEWORKS_RELEASE}" "${abseil_absl_node_hash_set_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_node_hash_set_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_node_hash_set_DEPS_TARGET)
            add_library(abseil_absl_node_hash_set_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_node_hash_set_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_node_hash_set_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_node_hash_set_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_node_hash_set_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_node_hash_set_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_node_hash_set_LIBS_RELEASE}"
                                      "${abseil_absl_node_hash_set_LIB_DIRS_RELEASE}"
                                      abseil_absl_node_hash_set_DEPS_TARGET
                                      abseil_absl_node_hash_set_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_node_hash_set")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::node_hash_set
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_node_hash_set_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_node_hash_set_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_node_hash_set_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::node_hash_set
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_node_hash_set_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::node_hash_set PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_node_hash_set_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::node_hash_set PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_node_hash_set_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::node_hash_set PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_node_hash_set_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::node_hash_set PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_node_hash_set_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::flat_hash_set #############

        set(abseil_absl_flat_hash_set_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_flat_hash_set_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_flat_hash_set_FRAMEWORKS_RELEASE}" "${abseil_absl_flat_hash_set_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_flat_hash_set_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_flat_hash_set_DEPS_TARGET)
            add_library(abseil_absl_flat_hash_set_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_flat_hash_set_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_flat_hash_set_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_flat_hash_set_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_flat_hash_set_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_flat_hash_set_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_flat_hash_set_LIBS_RELEASE}"
                                      "${abseil_absl_flat_hash_set_LIB_DIRS_RELEASE}"
                                      abseil_absl_flat_hash_set_DEPS_TARGET
                                      abseil_absl_flat_hash_set_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_flat_hash_set")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::flat_hash_set
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_flat_hash_set_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_flat_hash_set_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_flat_hash_set_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::flat_hash_set
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_flat_hash_set_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::flat_hash_set PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_flat_hash_set_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::flat_hash_set PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_flat_hash_set_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::flat_hash_set PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_flat_hash_set_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::flat_hash_set PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_flat_hash_set_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::statusor #############

        set(abseil_absl_statusor_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_statusor_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_statusor_FRAMEWORKS_RELEASE}" "${abseil_absl_statusor_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_statusor_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_statusor_DEPS_TARGET)
            add_library(abseil_absl_statusor_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_statusor_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_statusor_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_statusor_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_statusor_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_statusor_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_statusor_LIBS_RELEASE}"
                                      "${abseil_absl_statusor_LIB_DIRS_RELEASE}"
                                      abseil_absl_statusor_DEPS_TARGET
                                      abseil_absl_statusor_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_statusor")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::statusor
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_statusor_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_statusor_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_statusor_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::statusor
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_statusor_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::statusor PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_statusor_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::statusor PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_statusor_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::statusor PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_statusor_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::statusor PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_statusor_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::status #############

        set(abseil_absl_status_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_status_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_status_FRAMEWORKS_RELEASE}" "${abseil_absl_status_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_status_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_status_DEPS_TARGET)
            add_library(abseil_absl_status_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_status_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_status_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_status_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_status_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_status_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_status_LIBS_RELEASE}"
                                      "${abseil_absl_status_LIB_DIRS_RELEASE}"
                                      abseil_absl_status_DEPS_TARGET
                                      abseil_absl_status_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_status")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::status
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_status_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_status_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_status_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::status
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_status_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::status PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_status_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::status PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_status_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::status PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_status_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::status PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_status_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::random_random #############

        set(abseil_absl_random_random_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_random_random_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_random_random_FRAMEWORKS_RELEASE}" "${abseil_absl_random_random_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_random_random_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_random_random_DEPS_TARGET)
            add_library(abseil_absl_random_random_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_random_random_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_random_random_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_random_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_random_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_random_random_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_random_random_LIBS_RELEASE}"
                                      "${abseil_absl_random_random_LIB_DIRS_RELEASE}"
                                      abseil_absl_random_random_DEPS_TARGET
                                      abseil_absl_random_random_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_random_random")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::random_random
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_random_random_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_random_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_random_random_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::random_random
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_random_random_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::random_random PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_random_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::random_random PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_random_random_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::random_random PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_random_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::random_random PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_random_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::raw_hash_set #############

        set(abseil_absl_raw_hash_set_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_raw_hash_set_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_raw_hash_set_FRAMEWORKS_RELEASE}" "${abseil_absl_raw_hash_set_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_raw_hash_set_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_raw_hash_set_DEPS_TARGET)
            add_library(abseil_absl_raw_hash_set_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_raw_hash_set_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_raw_hash_set_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_raw_hash_set_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_raw_hash_set_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_raw_hash_set_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_raw_hash_set_LIBS_RELEASE}"
                                      "${abseil_absl_raw_hash_set_LIB_DIRS_RELEASE}"
                                      abseil_absl_raw_hash_set_DEPS_TARGET
                                      abseil_absl_raw_hash_set_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_raw_hash_set")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::raw_hash_set
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_raw_hash_set_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_raw_hash_set_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_raw_hash_set_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::raw_hash_set
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_raw_hash_set_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::raw_hash_set PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_raw_hash_set_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::raw_hash_set PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_raw_hash_set_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::raw_hash_set PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_raw_hash_set_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::raw_hash_set PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_raw_hash_set_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::hashtablez_sampler #############

        set(abseil_absl_hashtablez_sampler_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_hashtablez_sampler_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_hashtablez_sampler_FRAMEWORKS_RELEASE}" "${abseil_absl_hashtablez_sampler_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_hashtablez_sampler_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_hashtablez_sampler_DEPS_TARGET)
            add_library(abseil_absl_hashtablez_sampler_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_hashtablez_sampler_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_hashtablez_sampler_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_hashtablez_sampler_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_hashtablez_sampler_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_hashtablez_sampler_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_hashtablez_sampler_LIBS_RELEASE}"
                                      "${abseil_absl_hashtablez_sampler_LIB_DIRS_RELEASE}"
                                      abseil_absl_hashtablez_sampler_DEPS_TARGET
                                      abseil_absl_hashtablez_sampler_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_hashtablez_sampler")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::hashtablez_sampler
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_hashtablez_sampler_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_hashtablez_sampler_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_hashtablez_sampler_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::hashtablez_sampler
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_hashtablez_sampler_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::hashtablez_sampler PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_hashtablez_sampler_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::hashtablez_sampler PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_hashtablez_sampler_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::hashtablez_sampler PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_hashtablez_sampler_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::hashtablez_sampler PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_hashtablez_sampler_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::hash_function_defaults #############

        set(abseil_absl_hash_function_defaults_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_hash_function_defaults_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_hash_function_defaults_FRAMEWORKS_RELEASE}" "${abseil_absl_hash_function_defaults_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_hash_function_defaults_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_hash_function_defaults_DEPS_TARGET)
            add_library(abseil_absl_hash_function_defaults_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_hash_function_defaults_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_hash_function_defaults_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_hash_function_defaults_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_hash_function_defaults_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_hash_function_defaults_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_hash_function_defaults_LIBS_RELEASE}"
                                      "${abseil_absl_hash_function_defaults_LIB_DIRS_RELEASE}"
                                      abseil_absl_hash_function_defaults_DEPS_TARGET
                                      abseil_absl_hash_function_defaults_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_hash_function_defaults")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::hash_function_defaults
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_hash_function_defaults_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_hash_function_defaults_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_hash_function_defaults_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::hash_function_defaults
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_hash_function_defaults_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::hash_function_defaults PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_hash_function_defaults_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::hash_function_defaults PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_hash_function_defaults_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::hash_function_defaults PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_hash_function_defaults_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::hash_function_defaults PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_hash_function_defaults_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::btree #############

        set(abseil_absl_btree_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_btree_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_btree_FRAMEWORKS_RELEASE}" "${abseil_absl_btree_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_btree_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_btree_DEPS_TARGET)
            add_library(abseil_absl_btree_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_btree_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_btree_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_btree_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_btree_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_btree_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_btree_LIBS_RELEASE}"
                                      "${abseil_absl_btree_LIB_DIRS_RELEASE}"
                                      abseil_absl_btree_DEPS_TARGET
                                      abseil_absl_btree_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_btree")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::btree
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_btree_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_btree_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_btree_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::btree
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_btree_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::btree PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_btree_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::btree PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_btree_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::btree PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_btree_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::btree PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_btree_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::cord #############

        set(abseil_absl_cord_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_cord_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_cord_FRAMEWORKS_RELEASE}" "${abseil_absl_cord_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_cord_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_cord_DEPS_TARGET)
            add_library(abseil_absl_cord_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_cord_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_cord_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_cord_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_cord_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_cord_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_cord_LIBS_RELEASE}"
                                      "${abseil_absl_cord_LIB_DIRS_RELEASE}"
                                      abseil_absl_cord_DEPS_TARGET
                                      abseil_absl_cord_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_cord")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::cord
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_cord_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_cord_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_cord_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::cord
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_cord_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::cord PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_cord_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::cord PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_cord_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::cord PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_cord_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::cord PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_cord_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::cordz_update_scope #############

        set(abseil_absl_cordz_update_scope_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_cordz_update_scope_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_cordz_update_scope_FRAMEWORKS_RELEASE}" "${abseil_absl_cordz_update_scope_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_cordz_update_scope_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_cordz_update_scope_DEPS_TARGET)
            add_library(abseil_absl_cordz_update_scope_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_cordz_update_scope_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_cordz_update_scope_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_cordz_update_scope_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_cordz_update_scope_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_cordz_update_scope_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_cordz_update_scope_LIBS_RELEASE}"
                                      "${abseil_absl_cordz_update_scope_LIB_DIRS_RELEASE}"
                                      abseil_absl_cordz_update_scope_DEPS_TARGET
                                      abseil_absl_cordz_update_scope_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_cordz_update_scope")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::cordz_update_scope
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_cordz_update_scope_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_cordz_update_scope_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_cordz_update_scope_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::cordz_update_scope
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_cordz_update_scope_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::cordz_update_scope PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_cordz_update_scope_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::cordz_update_scope PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_cordz_update_scope_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::cordz_update_scope PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_cordz_update_scope_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::cordz_update_scope PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_cordz_update_scope_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::cordz_sample_token #############

        set(abseil_absl_cordz_sample_token_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_cordz_sample_token_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_cordz_sample_token_FRAMEWORKS_RELEASE}" "${abseil_absl_cordz_sample_token_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_cordz_sample_token_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_cordz_sample_token_DEPS_TARGET)
            add_library(abseil_absl_cordz_sample_token_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_cordz_sample_token_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_cordz_sample_token_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_cordz_sample_token_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_cordz_sample_token_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_cordz_sample_token_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_cordz_sample_token_LIBS_RELEASE}"
                                      "${abseil_absl_cordz_sample_token_LIB_DIRS_RELEASE}"
                                      abseil_absl_cordz_sample_token_DEPS_TARGET
                                      abseil_absl_cordz_sample_token_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_cordz_sample_token")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::cordz_sample_token
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_cordz_sample_token_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_cordz_sample_token_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_cordz_sample_token_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::cordz_sample_token
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_cordz_sample_token_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::cordz_sample_token PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_cordz_sample_token_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::cordz_sample_token PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_cordz_sample_token_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::cordz_sample_token PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_cordz_sample_token_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::cordz_sample_token PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_cordz_sample_token_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::cordz_info #############

        set(abseil_absl_cordz_info_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_cordz_info_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_cordz_info_FRAMEWORKS_RELEASE}" "${abseil_absl_cordz_info_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_cordz_info_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_cordz_info_DEPS_TARGET)
            add_library(abseil_absl_cordz_info_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_cordz_info_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_cordz_info_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_cordz_info_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_cordz_info_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_cordz_info_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_cordz_info_LIBS_RELEASE}"
                                      "${abseil_absl_cordz_info_LIB_DIRS_RELEASE}"
                                      abseil_absl_cordz_info_DEPS_TARGET
                                      abseil_absl_cordz_info_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_cordz_info")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::cordz_info
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_cordz_info_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_cordz_info_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_cordz_info_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::cordz_info
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_cordz_info_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::cordz_info PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_cordz_info_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::cordz_info PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_cordz_info_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::cordz_info PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_cordz_info_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::cordz_info PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_cordz_info_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::cordz_handle #############

        set(abseil_absl_cordz_handle_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_cordz_handle_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_cordz_handle_FRAMEWORKS_RELEASE}" "${abseil_absl_cordz_handle_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_cordz_handle_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_cordz_handle_DEPS_TARGET)
            add_library(abseil_absl_cordz_handle_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_cordz_handle_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_cordz_handle_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_cordz_handle_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_cordz_handle_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_cordz_handle_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_cordz_handle_LIBS_RELEASE}"
                                      "${abseil_absl_cordz_handle_LIB_DIRS_RELEASE}"
                                      abseil_absl_cordz_handle_DEPS_TARGET
                                      abseil_absl_cordz_handle_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_cordz_handle")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::cordz_handle
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_cordz_handle_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_cordz_handle_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_cordz_handle_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::cordz_handle
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_cordz_handle_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::cordz_handle PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_cordz_handle_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::cordz_handle PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_cordz_handle_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::cordz_handle PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_cordz_handle_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::cordz_handle PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_cordz_handle_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::cordz_statistics #############

        set(abseil_absl_cordz_statistics_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_cordz_statistics_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_cordz_statistics_FRAMEWORKS_RELEASE}" "${abseil_absl_cordz_statistics_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_cordz_statistics_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_cordz_statistics_DEPS_TARGET)
            add_library(abseil_absl_cordz_statistics_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_cordz_statistics_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_cordz_statistics_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_cordz_statistics_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_cordz_statistics_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_cordz_statistics_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_cordz_statistics_LIBS_RELEASE}"
                                      "${abseil_absl_cordz_statistics_LIB_DIRS_RELEASE}"
                                      abseil_absl_cordz_statistics_DEPS_TARGET
                                      abseil_absl_cordz_statistics_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_cordz_statistics")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::cordz_statistics
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_cordz_statistics_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_cordz_statistics_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_cordz_statistics_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::cordz_statistics
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_cordz_statistics_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::cordz_statistics PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_cordz_statistics_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::cordz_statistics PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_cordz_statistics_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::cordz_statistics PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_cordz_statistics_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::cordz_statistics PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_cordz_statistics_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::random_internal_distribution_test_util #############

        set(abseil_absl_random_internal_distribution_test_util_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_random_internal_distribution_test_util_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_random_internal_distribution_test_util_FRAMEWORKS_RELEASE}" "${abseil_absl_random_internal_distribution_test_util_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_random_internal_distribution_test_util_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_random_internal_distribution_test_util_DEPS_TARGET)
            add_library(abseil_absl_random_internal_distribution_test_util_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_random_internal_distribution_test_util_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_distribution_test_util_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_distribution_test_util_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_distribution_test_util_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_random_internal_distribution_test_util_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_random_internal_distribution_test_util_LIBS_RELEASE}"
                                      "${abseil_absl_random_internal_distribution_test_util_LIB_DIRS_RELEASE}"
                                      abseil_absl_random_internal_distribution_test_util_DEPS_TARGET
                                      abseil_absl_random_internal_distribution_test_util_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_random_internal_distribution_test_util")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::random_internal_distribution_test_util
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_distribution_test_util_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_distribution_test_util_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_random_internal_distribution_test_util_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::random_internal_distribution_test_util
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_random_internal_distribution_test_util_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::random_internal_distribution_test_util PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_distribution_test_util_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_distribution_test_util PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_distribution_test_util_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_distribution_test_util PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_distribution_test_util_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_distribution_test_util PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_distribution_test_util_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::random_seed_sequences #############

        set(abseil_absl_random_seed_sequences_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_random_seed_sequences_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_random_seed_sequences_FRAMEWORKS_RELEASE}" "${abseil_absl_random_seed_sequences_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_random_seed_sequences_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_random_seed_sequences_DEPS_TARGET)
            add_library(abseil_absl_random_seed_sequences_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_random_seed_sequences_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_random_seed_sequences_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_seed_sequences_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_seed_sequences_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_random_seed_sequences_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_random_seed_sequences_LIBS_RELEASE}"
                                      "${abseil_absl_random_seed_sequences_LIB_DIRS_RELEASE}"
                                      abseil_absl_random_seed_sequences_DEPS_TARGET
                                      abseil_absl_random_seed_sequences_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_random_seed_sequences")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::random_seed_sequences
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_random_seed_sequences_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_seed_sequences_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_random_seed_sequences_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::random_seed_sequences
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_random_seed_sequences_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::random_seed_sequences PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_seed_sequences_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::random_seed_sequences PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_random_seed_sequences_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::random_seed_sequences PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_seed_sequences_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::random_seed_sequences PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_seed_sequences_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::sample_recorder #############

        set(abseil_absl_sample_recorder_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_sample_recorder_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_sample_recorder_FRAMEWORKS_RELEASE}" "${abseil_absl_sample_recorder_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_sample_recorder_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_sample_recorder_DEPS_TARGET)
            add_library(abseil_absl_sample_recorder_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_sample_recorder_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_sample_recorder_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_sample_recorder_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_sample_recorder_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_sample_recorder_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_sample_recorder_LIBS_RELEASE}"
                                      "${abseil_absl_sample_recorder_LIB_DIRS_RELEASE}"
                                      abseil_absl_sample_recorder_DEPS_TARGET
                                      abseil_absl_sample_recorder_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_sample_recorder")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::sample_recorder
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_sample_recorder_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_sample_recorder_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_sample_recorder_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::sample_recorder
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_sample_recorder_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::sample_recorder PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_sample_recorder_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::sample_recorder PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_sample_recorder_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::sample_recorder PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_sample_recorder_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::sample_recorder PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_sample_recorder_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::flags_internal #############

        set(abseil_absl_flags_internal_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_flags_internal_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_flags_internal_FRAMEWORKS_RELEASE}" "${abseil_absl_flags_internal_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_flags_internal_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_flags_internal_DEPS_TARGET)
            add_library(abseil_absl_flags_internal_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_flags_internal_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_flags_internal_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_flags_internal_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_flags_internal_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_flags_internal_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_flags_internal_LIBS_RELEASE}"
                                      "${abseil_absl_flags_internal_LIB_DIRS_RELEASE}"
                                      abseil_absl_flags_internal_DEPS_TARGET
                                      abseil_absl_flags_internal_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_flags_internal")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::flags_internal
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_flags_internal_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_flags_internal_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_flags_internal_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::flags_internal
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_flags_internal_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::flags_internal PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_flags_internal_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::flags_internal PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_flags_internal_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::flags_internal PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_flags_internal_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::flags_internal PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_flags_internal_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::flags_marshalling #############

        set(abseil_absl_flags_marshalling_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_flags_marshalling_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_flags_marshalling_FRAMEWORKS_RELEASE}" "${abseil_absl_flags_marshalling_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_flags_marshalling_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_flags_marshalling_DEPS_TARGET)
            add_library(abseil_absl_flags_marshalling_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_flags_marshalling_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_flags_marshalling_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_flags_marshalling_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_flags_marshalling_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_flags_marshalling_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_flags_marshalling_LIBS_RELEASE}"
                                      "${abseil_absl_flags_marshalling_LIB_DIRS_RELEASE}"
                                      abseil_absl_flags_marshalling_DEPS_TARGET
                                      abseil_absl_flags_marshalling_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_flags_marshalling")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::flags_marshalling
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_flags_marshalling_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_flags_marshalling_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_flags_marshalling_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::flags_marshalling
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_flags_marshalling_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::flags_marshalling PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_flags_marshalling_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::flags_marshalling PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_flags_marshalling_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::flags_marshalling PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_flags_marshalling_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::flags_marshalling PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_flags_marshalling_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::flags_config #############

        set(abseil_absl_flags_config_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_flags_config_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_flags_config_FRAMEWORKS_RELEASE}" "${abseil_absl_flags_config_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_flags_config_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_flags_config_DEPS_TARGET)
            add_library(abseil_absl_flags_config_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_flags_config_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_flags_config_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_flags_config_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_flags_config_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_flags_config_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_flags_config_LIBS_RELEASE}"
                                      "${abseil_absl_flags_config_LIB_DIRS_RELEASE}"
                                      abseil_absl_flags_config_DEPS_TARGET
                                      abseil_absl_flags_config_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_flags_config")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::flags_config
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_flags_config_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_flags_config_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_flags_config_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::flags_config
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_flags_config_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::flags_config PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_flags_config_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::flags_config PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_flags_config_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::flags_config PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_flags_config_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::flags_config PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_flags_config_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::flags_program_name #############

        set(abseil_absl_flags_program_name_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_flags_program_name_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_flags_program_name_FRAMEWORKS_RELEASE}" "${abseil_absl_flags_program_name_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_flags_program_name_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_flags_program_name_DEPS_TARGET)
            add_library(abseil_absl_flags_program_name_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_flags_program_name_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_flags_program_name_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_flags_program_name_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_flags_program_name_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_flags_program_name_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_flags_program_name_LIBS_RELEASE}"
                                      "${abseil_absl_flags_program_name_LIB_DIRS_RELEASE}"
                                      abseil_absl_flags_program_name_DEPS_TARGET
                                      abseil_absl_flags_program_name_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_flags_program_name")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::flags_program_name
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_flags_program_name_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_flags_program_name_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_flags_program_name_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::flags_program_name
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_flags_program_name_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::flags_program_name PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_flags_program_name_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::flags_program_name PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_flags_program_name_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::flags_program_name PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_flags_program_name_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::flags_program_name PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_flags_program_name_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::synchronization #############

        set(abseil_absl_synchronization_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_synchronization_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_synchronization_FRAMEWORKS_RELEASE}" "${abseil_absl_synchronization_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_synchronization_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_synchronization_DEPS_TARGET)
            add_library(abseil_absl_synchronization_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_synchronization_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_synchronization_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_synchronization_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_synchronization_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_synchronization_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_synchronization_LIBS_RELEASE}"
                                      "${abseil_absl_synchronization_LIB_DIRS_RELEASE}"
                                      abseil_absl_synchronization_DEPS_TARGET
                                      abseil_absl_synchronization_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_synchronization")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::synchronization
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_synchronization_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_synchronization_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_synchronization_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::synchronization
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_synchronization_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::synchronization PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_synchronization_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::synchronization PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_synchronization_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::synchronization PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_synchronization_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::synchronization PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_synchronization_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::kernel_timeout_internal #############

        set(abseil_absl_kernel_timeout_internal_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_kernel_timeout_internal_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_kernel_timeout_internal_FRAMEWORKS_RELEASE}" "${abseil_absl_kernel_timeout_internal_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_kernel_timeout_internal_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_kernel_timeout_internal_DEPS_TARGET)
            add_library(abseil_absl_kernel_timeout_internal_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_kernel_timeout_internal_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_kernel_timeout_internal_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_kernel_timeout_internal_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_kernel_timeout_internal_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_kernel_timeout_internal_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_kernel_timeout_internal_LIBS_RELEASE}"
                                      "${abseil_absl_kernel_timeout_internal_LIB_DIRS_RELEASE}"
                                      abseil_absl_kernel_timeout_internal_DEPS_TARGET
                                      abseil_absl_kernel_timeout_internal_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_kernel_timeout_internal")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::kernel_timeout_internal
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_kernel_timeout_internal_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_kernel_timeout_internal_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_kernel_timeout_internal_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::kernel_timeout_internal
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_kernel_timeout_internal_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::kernel_timeout_internal PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_kernel_timeout_internal_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::kernel_timeout_internal PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_kernel_timeout_internal_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::kernel_timeout_internal PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_kernel_timeout_internal_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::kernel_timeout_internal PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_kernel_timeout_internal_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::cord_internal #############

        set(abseil_absl_cord_internal_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_cord_internal_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_cord_internal_FRAMEWORKS_RELEASE}" "${abseil_absl_cord_internal_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_cord_internal_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_cord_internal_DEPS_TARGET)
            add_library(abseil_absl_cord_internal_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_cord_internal_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_cord_internal_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_cord_internal_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_cord_internal_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_cord_internal_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_cord_internal_LIBS_RELEASE}"
                                      "${abseil_absl_cord_internal_LIB_DIRS_RELEASE}"
                                      abseil_absl_cord_internal_DEPS_TARGET
                                      abseil_absl_cord_internal_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_cord_internal")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::cord_internal
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_cord_internal_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_cord_internal_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_cord_internal_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::cord_internal
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_cord_internal_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::cord_internal PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_cord_internal_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::cord_internal PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_cord_internal_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::cord_internal PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_cord_internal_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::cord_internal PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_cord_internal_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::str_format #############

        set(abseil_absl_str_format_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_str_format_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_str_format_FRAMEWORKS_RELEASE}" "${abseil_absl_str_format_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_str_format_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_str_format_DEPS_TARGET)
            add_library(abseil_absl_str_format_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_str_format_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_str_format_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_str_format_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_str_format_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_str_format_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_str_format_LIBS_RELEASE}"
                                      "${abseil_absl_str_format_LIB_DIRS_RELEASE}"
                                      abseil_absl_str_format_DEPS_TARGET
                                      abseil_absl_str_format_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_str_format")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::str_format
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_str_format_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_str_format_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_str_format_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::str_format
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_str_format_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::str_format PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_str_format_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::str_format PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_str_format_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::str_format PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_str_format_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::str_format PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_str_format_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::random_internal_nonsecure_base #############

        set(abseil_absl_random_internal_nonsecure_base_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_random_internal_nonsecure_base_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_random_internal_nonsecure_base_FRAMEWORKS_RELEASE}" "${abseil_absl_random_internal_nonsecure_base_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_random_internal_nonsecure_base_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_random_internal_nonsecure_base_DEPS_TARGET)
            add_library(abseil_absl_random_internal_nonsecure_base_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_random_internal_nonsecure_base_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_nonsecure_base_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_nonsecure_base_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_nonsecure_base_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_random_internal_nonsecure_base_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_random_internal_nonsecure_base_LIBS_RELEASE}"
                                      "${abseil_absl_random_internal_nonsecure_base_LIB_DIRS_RELEASE}"
                                      abseil_absl_random_internal_nonsecure_base_DEPS_TARGET
                                      abseil_absl_random_internal_nonsecure_base_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_random_internal_nonsecure_base")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::random_internal_nonsecure_base
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_nonsecure_base_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_nonsecure_base_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_random_internal_nonsecure_base_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::random_internal_nonsecure_base
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_random_internal_nonsecure_base_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::random_internal_nonsecure_base PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_nonsecure_base_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_nonsecure_base PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_nonsecure_base_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_nonsecure_base PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_nonsecure_base_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_nonsecure_base PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_nonsecure_base_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::random_internal_salted_seed_seq #############

        set(abseil_absl_random_internal_salted_seed_seq_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_random_internal_salted_seed_seq_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_random_internal_salted_seed_seq_FRAMEWORKS_RELEASE}" "${abseil_absl_random_internal_salted_seed_seq_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_random_internal_salted_seed_seq_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_random_internal_salted_seed_seq_DEPS_TARGET)
            add_library(abseil_absl_random_internal_salted_seed_seq_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_random_internal_salted_seed_seq_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_salted_seed_seq_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_salted_seed_seq_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_salted_seed_seq_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_random_internal_salted_seed_seq_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_random_internal_salted_seed_seq_LIBS_RELEASE}"
                                      "${abseil_absl_random_internal_salted_seed_seq_LIB_DIRS_RELEASE}"
                                      abseil_absl_random_internal_salted_seed_seq_DEPS_TARGET
                                      abseil_absl_random_internal_salted_seed_seq_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_random_internal_salted_seed_seq")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::random_internal_salted_seed_seq
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_salted_seed_seq_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_salted_seed_seq_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_random_internal_salted_seed_seq_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::random_internal_salted_seed_seq
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_random_internal_salted_seed_seq_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::random_internal_salted_seed_seq PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_salted_seed_seq_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_salted_seed_seq PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_salted_seed_seq_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_salted_seed_seq PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_salted_seed_seq_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_salted_seed_seq PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_salted_seed_seq_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::random_internal_pool_urbg #############

        set(abseil_absl_random_internal_pool_urbg_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_random_internal_pool_urbg_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_random_internal_pool_urbg_FRAMEWORKS_RELEASE}" "${abseil_absl_random_internal_pool_urbg_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_random_internal_pool_urbg_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_random_internal_pool_urbg_DEPS_TARGET)
            add_library(abseil_absl_random_internal_pool_urbg_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_random_internal_pool_urbg_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_pool_urbg_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_pool_urbg_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_pool_urbg_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_random_internal_pool_urbg_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_random_internal_pool_urbg_LIBS_RELEASE}"
                                      "${abseil_absl_random_internal_pool_urbg_LIB_DIRS_RELEASE}"
                                      abseil_absl_random_internal_pool_urbg_DEPS_TARGET
                                      abseil_absl_random_internal_pool_urbg_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_random_internal_pool_urbg")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::random_internal_pool_urbg
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_pool_urbg_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_pool_urbg_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_random_internal_pool_urbg_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::random_internal_pool_urbg
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_random_internal_pool_urbg_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::random_internal_pool_urbg PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_pool_urbg_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_pool_urbg PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_pool_urbg_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_pool_urbg PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_pool_urbg_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_pool_urbg PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_pool_urbg_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::random_internal_seed_material #############

        set(abseil_absl_random_internal_seed_material_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_random_internal_seed_material_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_random_internal_seed_material_FRAMEWORKS_RELEASE}" "${abseil_absl_random_internal_seed_material_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_random_internal_seed_material_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_random_internal_seed_material_DEPS_TARGET)
            add_library(abseil_absl_random_internal_seed_material_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_random_internal_seed_material_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_seed_material_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_seed_material_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_seed_material_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_random_internal_seed_material_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_random_internal_seed_material_LIBS_RELEASE}"
                                      "${abseil_absl_random_internal_seed_material_LIB_DIRS_RELEASE}"
                                      abseil_absl_random_internal_seed_material_DEPS_TARGET
                                      abseil_absl_random_internal_seed_material_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_random_internal_seed_material")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::random_internal_seed_material
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_seed_material_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_seed_material_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_random_internal_seed_material_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::random_internal_seed_material
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_random_internal_seed_material_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::random_internal_seed_material PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_seed_material_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_seed_material PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_seed_material_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_seed_material PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_seed_material_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_seed_material PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_seed_material_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::random_distributions #############

        set(abseil_absl_random_distributions_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_random_distributions_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_random_distributions_FRAMEWORKS_RELEASE}" "${abseil_absl_random_distributions_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_random_distributions_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_random_distributions_DEPS_TARGET)
            add_library(abseil_absl_random_distributions_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_random_distributions_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_random_distributions_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_distributions_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_distributions_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_random_distributions_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_random_distributions_LIBS_RELEASE}"
                                      "${abseil_absl_random_distributions_LIB_DIRS_RELEASE}"
                                      abseil_absl_random_distributions_DEPS_TARGET
                                      abseil_absl_random_distributions_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_random_distributions")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::random_distributions
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_random_distributions_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_distributions_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_random_distributions_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::random_distributions
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_random_distributions_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::random_distributions PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_distributions_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::random_distributions PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_random_distributions_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::random_distributions PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_distributions_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::random_distributions PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_distributions_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::hash #############

        set(abseil_absl_hash_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_hash_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_hash_FRAMEWORKS_RELEASE}" "${abseil_absl_hash_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_hash_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_hash_DEPS_TARGET)
            add_library(abseil_absl_hash_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_hash_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_hash_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_hash_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_hash_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_hash_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_hash_LIBS_RELEASE}"
                                      "${abseil_absl_hash_LIB_DIRS_RELEASE}"
                                      abseil_absl_hash_DEPS_TARGET
                                      abseil_absl_hash_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_hash")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::hash
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_hash_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_hash_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_hash_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::hash
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_hash_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::hash PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_hash_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::hash PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_hash_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::hash PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_hash_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::hash PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_hash_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::flags_private_handle_accessor #############

        set(abseil_absl_flags_private_handle_accessor_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_flags_private_handle_accessor_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_flags_private_handle_accessor_FRAMEWORKS_RELEASE}" "${abseil_absl_flags_private_handle_accessor_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_flags_private_handle_accessor_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_flags_private_handle_accessor_DEPS_TARGET)
            add_library(abseil_absl_flags_private_handle_accessor_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_flags_private_handle_accessor_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_flags_private_handle_accessor_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_flags_private_handle_accessor_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_flags_private_handle_accessor_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_flags_private_handle_accessor_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_flags_private_handle_accessor_LIBS_RELEASE}"
                                      "${abseil_absl_flags_private_handle_accessor_LIB_DIRS_RELEASE}"
                                      abseil_absl_flags_private_handle_accessor_DEPS_TARGET
                                      abseil_absl_flags_private_handle_accessor_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_flags_private_handle_accessor")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::flags_private_handle_accessor
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_flags_private_handle_accessor_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_flags_private_handle_accessor_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_flags_private_handle_accessor_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::flags_private_handle_accessor
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_flags_private_handle_accessor_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::flags_private_handle_accessor PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_flags_private_handle_accessor_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::flags_private_handle_accessor PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_flags_private_handle_accessor_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::flags_private_handle_accessor PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_flags_private_handle_accessor_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::flags_private_handle_accessor PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_flags_private_handle_accessor_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::flags_commandlineflag #############

        set(abseil_absl_flags_commandlineflag_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_flags_commandlineflag_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_flags_commandlineflag_FRAMEWORKS_RELEASE}" "${abseil_absl_flags_commandlineflag_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_flags_commandlineflag_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_flags_commandlineflag_DEPS_TARGET)
            add_library(abseil_absl_flags_commandlineflag_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_flags_commandlineflag_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_flags_commandlineflag_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_flags_commandlineflag_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_flags_commandlineflag_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_flags_commandlineflag_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_flags_commandlineflag_LIBS_RELEASE}"
                                      "${abseil_absl_flags_commandlineflag_LIB_DIRS_RELEASE}"
                                      abseil_absl_flags_commandlineflag_DEPS_TARGET
                                      abseil_absl_flags_commandlineflag_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_flags_commandlineflag")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::flags_commandlineflag
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_flags_commandlineflag_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_flags_commandlineflag_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_flags_commandlineflag_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::flags_commandlineflag
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_flags_commandlineflag_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::flags_commandlineflag PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_flags_commandlineflag_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::flags_commandlineflag PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_flags_commandlineflag_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::flags_commandlineflag PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_flags_commandlineflag_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::flags_commandlineflag PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_flags_commandlineflag_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::flags_path_util #############

        set(abseil_absl_flags_path_util_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_flags_path_util_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_flags_path_util_FRAMEWORKS_RELEASE}" "${abseil_absl_flags_path_util_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_flags_path_util_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_flags_path_util_DEPS_TARGET)
            add_library(abseil_absl_flags_path_util_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_flags_path_util_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_flags_path_util_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_flags_path_util_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_flags_path_util_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_flags_path_util_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_flags_path_util_LIBS_RELEASE}"
                                      "${abseil_absl_flags_path_util_LIB_DIRS_RELEASE}"
                                      abseil_absl_flags_path_util_DEPS_TARGET
                                      abseil_absl_flags_path_util_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_flags_path_util")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::flags_path_util
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_flags_path_util_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_flags_path_util_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_flags_path_util_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::flags_path_util
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_flags_path_util_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::flags_path_util PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_flags_path_util_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::flags_path_util PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_flags_path_util_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::flags_path_util PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_flags_path_util_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::flags_path_util PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_flags_path_util_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::failure_signal_handler #############

        set(abseil_absl_failure_signal_handler_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_failure_signal_handler_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_failure_signal_handler_FRAMEWORKS_RELEASE}" "${abseil_absl_failure_signal_handler_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_failure_signal_handler_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_failure_signal_handler_DEPS_TARGET)
            add_library(abseil_absl_failure_signal_handler_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_failure_signal_handler_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_failure_signal_handler_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_failure_signal_handler_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_failure_signal_handler_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_failure_signal_handler_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_failure_signal_handler_LIBS_RELEASE}"
                                      "${abseil_absl_failure_signal_handler_LIB_DIRS_RELEASE}"
                                      abseil_absl_failure_signal_handler_DEPS_TARGET
                                      abseil_absl_failure_signal_handler_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_failure_signal_handler")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::failure_signal_handler
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_failure_signal_handler_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_failure_signal_handler_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_failure_signal_handler_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::failure_signal_handler
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_failure_signal_handler_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::failure_signal_handler PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_failure_signal_handler_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::failure_signal_handler PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_failure_signal_handler_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::failure_signal_handler PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_failure_signal_handler_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::failure_signal_handler PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_failure_signal_handler_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::examine_stack #############

        set(abseil_absl_examine_stack_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_examine_stack_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_examine_stack_FRAMEWORKS_RELEASE}" "${abseil_absl_examine_stack_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_examine_stack_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_examine_stack_DEPS_TARGET)
            add_library(abseil_absl_examine_stack_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_examine_stack_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_examine_stack_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_examine_stack_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_examine_stack_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_examine_stack_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_examine_stack_LIBS_RELEASE}"
                                      "${abseil_absl_examine_stack_LIB_DIRS_RELEASE}"
                                      abseil_absl_examine_stack_DEPS_TARGET
                                      abseil_absl_examine_stack_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_examine_stack")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::examine_stack
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_examine_stack_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_examine_stack_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_examine_stack_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::examine_stack
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_examine_stack_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::examine_stack PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_examine_stack_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::examine_stack PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_examine_stack_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::examine_stack PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_examine_stack_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::examine_stack PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_examine_stack_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::symbolize #############

        set(abseil_absl_symbolize_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_symbolize_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_symbolize_FRAMEWORKS_RELEASE}" "${abseil_absl_symbolize_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_symbolize_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_symbolize_DEPS_TARGET)
            add_library(abseil_absl_symbolize_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_symbolize_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_symbolize_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_symbolize_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_symbolize_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_symbolize_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_symbolize_LIBS_RELEASE}"
                                      "${abseil_absl_symbolize_LIB_DIRS_RELEASE}"
                                      abseil_absl_symbolize_DEPS_TARGET
                                      abseil_absl_symbolize_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_symbolize")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::symbolize
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_symbolize_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_symbolize_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_symbolize_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::symbolize
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_symbolize_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::symbolize PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_symbolize_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::symbolize PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_symbolize_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::symbolize PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_symbolize_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::symbolize PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_symbolize_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::layout #############

        set(abseil_absl_layout_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_layout_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_layout_FRAMEWORKS_RELEASE}" "${abseil_absl_layout_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_layout_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_layout_DEPS_TARGET)
            add_library(abseil_absl_layout_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_layout_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_layout_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_layout_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_layout_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_layout_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_layout_LIBS_RELEASE}"
                                      "${abseil_absl_layout_LIB_DIRS_RELEASE}"
                                      abseil_absl_layout_DEPS_TARGET
                                      abseil_absl_layout_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_layout")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::layout
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_layout_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_layout_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_layout_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::layout
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_layout_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::layout PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_layout_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::layout PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_layout_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::layout PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_layout_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::layout PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_layout_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::any #############

        set(abseil_absl_any_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_any_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_any_FRAMEWORKS_RELEASE}" "${abseil_absl_any_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_any_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_any_DEPS_TARGET)
            add_library(abseil_absl_any_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_any_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_any_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_any_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_any_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_any_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_any_LIBS_RELEASE}"
                                      "${abseil_absl_any_LIB_DIRS_RELEASE}"
                                      abseil_absl_any_DEPS_TARGET
                                      abseil_absl_any_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_any")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::any
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_any_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_any_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_any_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::any
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_any_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::any PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_any_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::any PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_any_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::any PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_any_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::any PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_any_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::time #############

        set(abseil_absl_time_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_time_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_time_FRAMEWORKS_RELEASE}" "${abseil_absl_time_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_time_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_time_DEPS_TARGET)
            add_library(abseil_absl_time_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_time_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_time_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_time_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_time_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_time_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_time_LIBS_RELEASE}"
                                      "${abseil_absl_time_LIB_DIRS_RELEASE}"
                                      abseil_absl_time_DEPS_TARGET
                                      abseil_absl_time_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_time")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::time
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_time_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_time_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_time_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::time
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_time_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::time PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_time_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::time PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_time_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::time PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_time_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::time PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_time_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::graphcycles_internal #############

        set(abseil_absl_graphcycles_internal_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_graphcycles_internal_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_graphcycles_internal_FRAMEWORKS_RELEASE}" "${abseil_absl_graphcycles_internal_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_graphcycles_internal_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_graphcycles_internal_DEPS_TARGET)
            add_library(abseil_absl_graphcycles_internal_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_graphcycles_internal_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_graphcycles_internal_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_graphcycles_internal_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_graphcycles_internal_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_graphcycles_internal_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_graphcycles_internal_LIBS_RELEASE}"
                                      "${abseil_absl_graphcycles_internal_LIB_DIRS_RELEASE}"
                                      abseil_absl_graphcycles_internal_DEPS_TARGET
                                      abseil_absl_graphcycles_internal_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_graphcycles_internal")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::graphcycles_internal
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_graphcycles_internal_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_graphcycles_internal_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_graphcycles_internal_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::graphcycles_internal
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_graphcycles_internal_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::graphcycles_internal PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_graphcycles_internal_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::graphcycles_internal PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_graphcycles_internal_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::graphcycles_internal PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_graphcycles_internal_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::graphcycles_internal PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_graphcycles_internal_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::str_format_internal #############

        set(abseil_absl_str_format_internal_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_str_format_internal_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_str_format_internal_FRAMEWORKS_RELEASE}" "${abseil_absl_str_format_internal_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_str_format_internal_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_str_format_internal_DEPS_TARGET)
            add_library(abseil_absl_str_format_internal_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_str_format_internal_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_str_format_internal_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_str_format_internal_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_str_format_internal_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_str_format_internal_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_str_format_internal_LIBS_RELEASE}"
                                      "${abseil_absl_str_format_internal_LIB_DIRS_RELEASE}"
                                      abseil_absl_str_format_internal_DEPS_TARGET
                                      abseil_absl_str_format_internal_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_str_format_internal")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::str_format_internal
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_str_format_internal_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_str_format_internal_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_str_format_internal_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::str_format_internal
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_str_format_internal_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::str_format_internal PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_str_format_internal_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::str_format_internal PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_str_format_internal_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::str_format_internal PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_str_format_internal_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::str_format_internal PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_str_format_internal_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::strings #############

        set(abseil_absl_strings_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_strings_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_strings_FRAMEWORKS_RELEASE}" "${abseil_absl_strings_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_strings_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_strings_DEPS_TARGET)
            add_library(abseil_absl_strings_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_strings_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_strings_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_strings_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_strings_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_strings_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_strings_LIBS_RELEASE}"
                                      "${abseil_absl_strings_LIB_DIRS_RELEASE}"
                                      abseil_absl_strings_DEPS_TARGET
                                      abseil_absl_strings_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_strings")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::strings
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_strings_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_strings_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_strings_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::strings
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_strings_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::strings PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_strings_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::strings PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_strings_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::strings PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_strings_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::strings PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_strings_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::random_internal_randen_engine #############

        set(abseil_absl_random_internal_randen_engine_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_random_internal_randen_engine_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_random_internal_randen_engine_FRAMEWORKS_RELEASE}" "${abseil_absl_random_internal_randen_engine_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_random_internal_randen_engine_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_random_internal_randen_engine_DEPS_TARGET)
            add_library(abseil_absl_random_internal_randen_engine_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_random_internal_randen_engine_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_randen_engine_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_randen_engine_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_randen_engine_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_random_internal_randen_engine_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_random_internal_randen_engine_LIBS_RELEASE}"
                                      "${abseil_absl_random_internal_randen_engine_LIB_DIRS_RELEASE}"
                                      abseil_absl_random_internal_randen_engine_DEPS_TARGET
                                      abseil_absl_random_internal_randen_engine_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_random_internal_randen_engine")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::random_internal_randen_engine
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_randen_engine_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_randen_engine_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_random_internal_randen_engine_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::random_internal_randen_engine
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_random_internal_randen_engine_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::random_internal_randen_engine PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_randen_engine_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_randen_engine PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_randen_engine_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_randen_engine PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_randen_engine_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_randen_engine PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_randen_engine_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::random_internal_mock_helpers #############

        set(abseil_absl_random_internal_mock_helpers_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_random_internal_mock_helpers_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_random_internal_mock_helpers_FRAMEWORKS_RELEASE}" "${abseil_absl_random_internal_mock_helpers_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_random_internal_mock_helpers_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_random_internal_mock_helpers_DEPS_TARGET)
            add_library(abseil_absl_random_internal_mock_helpers_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_random_internal_mock_helpers_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_mock_helpers_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_mock_helpers_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_mock_helpers_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_random_internal_mock_helpers_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_random_internal_mock_helpers_LIBS_RELEASE}"
                                      "${abseil_absl_random_internal_mock_helpers_LIB_DIRS_RELEASE}"
                                      abseil_absl_random_internal_mock_helpers_DEPS_TARGET
                                      abseil_absl_random_internal_mock_helpers_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_random_internal_mock_helpers")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::random_internal_mock_helpers
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_mock_helpers_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_mock_helpers_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_random_internal_mock_helpers_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::random_internal_mock_helpers
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_random_internal_mock_helpers_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::random_internal_mock_helpers PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_mock_helpers_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_mock_helpers PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_mock_helpers_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_mock_helpers PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_mock_helpers_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_mock_helpers PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_mock_helpers_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::random_bit_gen_ref #############

        set(abseil_absl_random_bit_gen_ref_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_random_bit_gen_ref_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_random_bit_gen_ref_FRAMEWORKS_RELEASE}" "${abseil_absl_random_bit_gen_ref_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_random_bit_gen_ref_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_random_bit_gen_ref_DEPS_TARGET)
            add_library(abseil_absl_random_bit_gen_ref_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_random_bit_gen_ref_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_random_bit_gen_ref_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_bit_gen_ref_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_bit_gen_ref_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_random_bit_gen_ref_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_random_bit_gen_ref_LIBS_RELEASE}"
                                      "${abseil_absl_random_bit_gen_ref_LIB_DIRS_RELEASE}"
                                      abseil_absl_random_bit_gen_ref_DEPS_TARGET
                                      abseil_absl_random_bit_gen_ref_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_random_bit_gen_ref")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::random_bit_gen_ref
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_random_bit_gen_ref_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_bit_gen_ref_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_random_bit_gen_ref_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::random_bit_gen_ref
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_random_bit_gen_ref_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::random_bit_gen_ref PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_bit_gen_ref_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::random_bit_gen_ref PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_random_bit_gen_ref_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::random_bit_gen_ref PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_bit_gen_ref_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::random_bit_gen_ref PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_bit_gen_ref_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::malloc_internal #############

        set(abseil_absl_malloc_internal_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_malloc_internal_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_malloc_internal_FRAMEWORKS_RELEASE}" "${abseil_absl_malloc_internal_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_malloc_internal_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_malloc_internal_DEPS_TARGET)
            add_library(abseil_absl_malloc_internal_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_malloc_internal_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_malloc_internal_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_malloc_internal_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_malloc_internal_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_malloc_internal_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_malloc_internal_LIBS_RELEASE}"
                                      "${abseil_absl_malloc_internal_LIB_DIRS_RELEASE}"
                                      abseil_absl_malloc_internal_DEPS_TARGET
                                      abseil_absl_malloc_internal_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_malloc_internal")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::malloc_internal
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_malloc_internal_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_malloc_internal_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_malloc_internal_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::malloc_internal
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_malloc_internal_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::malloc_internal PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_malloc_internal_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::malloc_internal PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_malloc_internal_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::malloc_internal PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_malloc_internal_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::malloc_internal PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_malloc_internal_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::variant #############

        set(abseil_absl_variant_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_variant_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_variant_FRAMEWORKS_RELEASE}" "${abseil_absl_variant_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_variant_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_variant_DEPS_TARGET)
            add_library(abseil_absl_variant_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_variant_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_variant_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_variant_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_variant_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_variant_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_variant_LIBS_RELEASE}"
                                      "${abseil_absl_variant_LIB_DIRS_RELEASE}"
                                      abseil_absl_variant_DEPS_TARGET
                                      abseil_absl_variant_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_variant")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::variant
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_variant_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_variant_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_variant_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::variant
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_variant_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::variant PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_variant_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::variant PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_variant_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::variant PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_variant_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::variant PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_variant_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::optional #############

        set(abseil_absl_optional_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_optional_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_optional_FRAMEWORKS_RELEASE}" "${abseil_absl_optional_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_optional_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_optional_DEPS_TARGET)
            add_library(abseil_absl_optional_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_optional_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_optional_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_optional_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_optional_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_optional_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_optional_LIBS_RELEASE}"
                                      "${abseil_absl_optional_LIB_DIRS_RELEASE}"
                                      abseil_absl_optional_DEPS_TARGET
                                      abseil_absl_optional_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_optional")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::optional
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_optional_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_optional_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_optional_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::optional
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_optional_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::optional PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_optional_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::optional PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_optional_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::optional PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_optional_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::optional PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_optional_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::bad_any_cast #############

        set(abseil_absl_bad_any_cast_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_bad_any_cast_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_bad_any_cast_FRAMEWORKS_RELEASE}" "${abseil_absl_bad_any_cast_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_bad_any_cast_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_bad_any_cast_DEPS_TARGET)
            add_library(abseil_absl_bad_any_cast_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_bad_any_cast_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_bad_any_cast_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_bad_any_cast_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_bad_any_cast_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_bad_any_cast_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_bad_any_cast_LIBS_RELEASE}"
                                      "${abseil_absl_bad_any_cast_LIB_DIRS_RELEASE}"
                                      abseil_absl_bad_any_cast_DEPS_TARGET
                                      abseil_absl_bad_any_cast_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_bad_any_cast")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::bad_any_cast
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_bad_any_cast_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_bad_any_cast_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_bad_any_cast_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::bad_any_cast
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_bad_any_cast_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::bad_any_cast PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_bad_any_cast_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::bad_any_cast PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_bad_any_cast_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::bad_any_cast PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_bad_any_cast_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::bad_any_cast PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_bad_any_cast_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::strings_internal #############

        set(abseil_absl_strings_internal_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_strings_internal_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_strings_internal_FRAMEWORKS_RELEASE}" "${abseil_absl_strings_internal_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_strings_internal_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_strings_internal_DEPS_TARGET)
            add_library(abseil_absl_strings_internal_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_strings_internal_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_strings_internal_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_strings_internal_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_strings_internal_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_strings_internal_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_strings_internal_LIBS_RELEASE}"
                                      "${abseil_absl_strings_internal_LIB_DIRS_RELEASE}"
                                      abseil_absl_strings_internal_DEPS_TARGET
                                      abseil_absl_strings_internal_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_strings_internal")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::strings_internal
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_strings_internal_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_strings_internal_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_strings_internal_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::strings_internal
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_strings_internal_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::strings_internal PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_strings_internal_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::strings_internal PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_strings_internal_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::strings_internal PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_strings_internal_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::strings_internal PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_strings_internal_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::random_internal_randen #############

        set(abseil_absl_random_internal_randen_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_random_internal_randen_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_random_internal_randen_FRAMEWORKS_RELEASE}" "${abseil_absl_random_internal_randen_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_random_internal_randen_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_random_internal_randen_DEPS_TARGET)
            add_library(abseil_absl_random_internal_randen_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_random_internal_randen_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_randen_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_randen_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_randen_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_random_internal_randen_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_random_internal_randen_LIBS_RELEASE}"
                                      "${abseil_absl_random_internal_randen_LIB_DIRS_RELEASE}"
                                      abseil_absl_random_internal_randen_DEPS_TARGET
                                      abseil_absl_random_internal_randen_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_random_internal_randen")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::random_internal_randen
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_randen_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_randen_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_random_internal_randen_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::random_internal_randen
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_random_internal_randen_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::random_internal_randen PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_randen_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_randen PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_randen_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_randen PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_randen_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_randen PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_randen_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::random_internal_distribution_caller #############

        set(abseil_absl_random_internal_distribution_caller_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_random_internal_distribution_caller_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_random_internal_distribution_caller_FRAMEWORKS_RELEASE}" "${abseil_absl_random_internal_distribution_caller_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_random_internal_distribution_caller_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_random_internal_distribution_caller_DEPS_TARGET)
            add_library(abseil_absl_random_internal_distribution_caller_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_random_internal_distribution_caller_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_distribution_caller_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_distribution_caller_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_distribution_caller_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_random_internal_distribution_caller_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_random_internal_distribution_caller_LIBS_RELEASE}"
                                      "${abseil_absl_random_internal_distribution_caller_LIB_DIRS_RELEASE}"
                                      abseil_absl_random_internal_distribution_caller_DEPS_TARGET
                                      abseil_absl_random_internal_distribution_caller_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_random_internal_distribution_caller")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::random_internal_distribution_caller
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_distribution_caller_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_distribution_caller_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_random_internal_distribution_caller_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::random_internal_distribution_caller
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_random_internal_distribution_caller_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::random_internal_distribution_caller PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_distribution_caller_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_distribution_caller PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_distribution_caller_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_distribution_caller PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_distribution_caller_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_distribution_caller PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_distribution_caller_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::low_level_hash #############

        set(abseil_absl_low_level_hash_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_low_level_hash_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_low_level_hash_FRAMEWORKS_RELEASE}" "${abseil_absl_low_level_hash_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_low_level_hash_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_low_level_hash_DEPS_TARGET)
            add_library(abseil_absl_low_level_hash_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_low_level_hash_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_low_level_hash_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_low_level_hash_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_low_level_hash_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_low_level_hash_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_low_level_hash_LIBS_RELEASE}"
                                      "${abseil_absl_low_level_hash_LIB_DIRS_RELEASE}"
                                      abseil_absl_low_level_hash_DEPS_TARGET
                                      abseil_absl_low_level_hash_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_low_level_hash")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::low_level_hash
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_low_level_hash_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_low_level_hash_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_low_level_hash_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::low_level_hash
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_low_level_hash_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::low_level_hash PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_low_level_hash_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::low_level_hash PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_low_level_hash_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::low_level_hash PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_low_level_hash_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::low_level_hash PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_low_level_hash_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::city #############

        set(abseil_absl_city_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_city_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_city_FRAMEWORKS_RELEASE}" "${abseil_absl_city_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_city_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_city_DEPS_TARGET)
            add_library(abseil_absl_city_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_city_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_city_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_city_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_city_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_city_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_city_LIBS_RELEASE}"
                                      "${abseil_absl_city_LIB_DIRS_RELEASE}"
                                      abseil_absl_city_DEPS_TARGET
                                      abseil_absl_city_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_city")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::city
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_city_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_city_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_city_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::city
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_city_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::city PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_city_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::city PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_city_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::city PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_city_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::city PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_city_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::bind_front #############

        set(abseil_absl_bind_front_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_bind_front_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_bind_front_FRAMEWORKS_RELEASE}" "${abseil_absl_bind_front_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_bind_front_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_bind_front_DEPS_TARGET)
            add_library(abseil_absl_bind_front_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_bind_front_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_bind_front_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_bind_front_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_bind_front_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_bind_front_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_bind_front_LIBS_RELEASE}"
                                      "${abseil_absl_bind_front_LIB_DIRS_RELEASE}"
                                      abseil_absl_bind_front_DEPS_TARGET
                                      abseil_absl_bind_front_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_bind_front")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::bind_front
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_bind_front_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_bind_front_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_bind_front_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::bind_front
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_bind_front_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::bind_front PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_bind_front_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::bind_front PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_bind_front_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::bind_front PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_bind_front_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::bind_front PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_bind_front_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::any_invocable #############

        set(abseil_absl_any_invocable_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_any_invocable_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_any_invocable_FRAMEWORKS_RELEASE}" "${abseil_absl_any_invocable_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_any_invocable_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_any_invocable_DEPS_TARGET)
            add_library(abseil_absl_any_invocable_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_any_invocable_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_any_invocable_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_any_invocable_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_any_invocable_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_any_invocable_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_any_invocable_LIBS_RELEASE}"
                                      "${abseil_absl_any_invocable_LIB_DIRS_RELEASE}"
                                      abseil_absl_any_invocable_DEPS_TARGET
                                      abseil_absl_any_invocable_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_any_invocable")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::any_invocable
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_any_invocable_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_any_invocable_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_any_invocable_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::any_invocable
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_any_invocable_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::any_invocable PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_any_invocable_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::any_invocable PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_any_invocable_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::any_invocable PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_any_invocable_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::any_invocable PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_any_invocable_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::debugging #############

        set(abseil_absl_debugging_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_debugging_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_debugging_FRAMEWORKS_RELEASE}" "${abseil_absl_debugging_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_debugging_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_debugging_DEPS_TARGET)
            add_library(abseil_absl_debugging_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_debugging_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_debugging_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_debugging_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_debugging_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_debugging_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_debugging_LIBS_RELEASE}"
                                      "${abseil_absl_debugging_LIB_DIRS_RELEASE}"
                                      abseil_absl_debugging_DEPS_TARGET
                                      abseil_absl_debugging_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_debugging")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::debugging
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_debugging_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_debugging_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_debugging_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::debugging
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_debugging_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::debugging PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_debugging_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::debugging PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_debugging_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::debugging PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_debugging_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::debugging PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_debugging_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::demangle_internal #############

        set(abseil_absl_demangle_internal_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_demangle_internal_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_demangle_internal_FRAMEWORKS_RELEASE}" "${abseil_absl_demangle_internal_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_demangle_internal_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_demangle_internal_DEPS_TARGET)
            add_library(abseil_absl_demangle_internal_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_demangle_internal_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_demangle_internal_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_demangle_internal_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_demangle_internal_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_demangle_internal_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_demangle_internal_LIBS_RELEASE}"
                                      "${abseil_absl_demangle_internal_LIB_DIRS_RELEASE}"
                                      abseil_absl_demangle_internal_DEPS_TARGET
                                      abseil_absl_demangle_internal_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_demangle_internal")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::demangle_internal
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_demangle_internal_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_demangle_internal_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_demangle_internal_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::demangle_internal
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_demangle_internal_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::demangle_internal PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_demangle_internal_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::demangle_internal PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_demangle_internal_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::demangle_internal PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_demangle_internal_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::demangle_internal PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_demangle_internal_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::stacktrace #############

        set(abseil_absl_stacktrace_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_stacktrace_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_stacktrace_FRAMEWORKS_RELEASE}" "${abseil_absl_stacktrace_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_stacktrace_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_stacktrace_DEPS_TARGET)
            add_library(abseil_absl_stacktrace_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_stacktrace_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_stacktrace_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_stacktrace_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_stacktrace_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_stacktrace_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_stacktrace_LIBS_RELEASE}"
                                      "${abseil_absl_stacktrace_LIB_DIRS_RELEASE}"
                                      abseil_absl_stacktrace_DEPS_TARGET
                                      abseil_absl_stacktrace_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_stacktrace")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::stacktrace
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_stacktrace_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_stacktrace_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_stacktrace_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::stacktrace
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_stacktrace_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::stacktrace PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_stacktrace_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::stacktrace PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_stacktrace_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::stacktrace PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_stacktrace_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::stacktrace PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_stacktrace_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::container_memory #############

        set(abseil_absl_container_memory_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_container_memory_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_container_memory_FRAMEWORKS_RELEASE}" "${abseil_absl_container_memory_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_container_memory_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_container_memory_DEPS_TARGET)
            add_library(abseil_absl_container_memory_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_container_memory_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_container_memory_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_container_memory_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_container_memory_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_container_memory_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_container_memory_LIBS_RELEASE}"
                                      "${abseil_absl_container_memory_LIB_DIRS_RELEASE}"
                                      abseil_absl_container_memory_DEPS_TARGET
                                      abseil_absl_container_memory_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_container_memory")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::container_memory
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_container_memory_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_container_memory_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_container_memory_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::container_memory
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_container_memory_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::container_memory PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_container_memory_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::container_memory PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_container_memory_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::container_memory PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_container_memory_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::container_memory PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_container_memory_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::inlined_vector #############

        set(abseil_absl_inlined_vector_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_inlined_vector_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_inlined_vector_FRAMEWORKS_RELEASE}" "${abseil_absl_inlined_vector_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_inlined_vector_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_inlined_vector_DEPS_TARGET)
            add_library(abseil_absl_inlined_vector_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_inlined_vector_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_inlined_vector_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_inlined_vector_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_inlined_vector_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_inlined_vector_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_inlined_vector_LIBS_RELEASE}"
                                      "${abseil_absl_inlined_vector_LIB_DIRS_RELEASE}"
                                      abseil_absl_inlined_vector_DEPS_TARGET
                                      abseil_absl_inlined_vector_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_inlined_vector")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::inlined_vector
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_inlined_vector_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_inlined_vector_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_inlined_vector_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::inlined_vector
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_inlined_vector_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::inlined_vector PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_inlined_vector_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::inlined_vector PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_inlined_vector_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::inlined_vector PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_inlined_vector_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::inlined_vector PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_inlined_vector_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::inlined_vector_internal #############

        set(abseil_absl_inlined_vector_internal_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_inlined_vector_internal_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_inlined_vector_internal_FRAMEWORKS_RELEASE}" "${abseil_absl_inlined_vector_internal_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_inlined_vector_internal_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_inlined_vector_internal_DEPS_TARGET)
            add_library(abseil_absl_inlined_vector_internal_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_inlined_vector_internal_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_inlined_vector_internal_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_inlined_vector_internal_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_inlined_vector_internal_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_inlined_vector_internal_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_inlined_vector_internal_LIBS_RELEASE}"
                                      "${abseil_absl_inlined_vector_internal_LIB_DIRS_RELEASE}"
                                      abseil_absl_inlined_vector_internal_DEPS_TARGET
                                      abseil_absl_inlined_vector_internal_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_inlined_vector_internal")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::inlined_vector_internal
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_inlined_vector_internal_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_inlined_vector_internal_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_inlined_vector_internal_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::inlined_vector_internal
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_inlined_vector_internal_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::inlined_vector_internal PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_inlined_vector_internal_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::inlined_vector_internal PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_inlined_vector_internal_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::inlined_vector_internal PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_inlined_vector_internal_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::inlined_vector_internal PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_inlined_vector_internal_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::fixed_array #############

        set(abseil_absl_fixed_array_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_fixed_array_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_fixed_array_FRAMEWORKS_RELEASE}" "${abseil_absl_fixed_array_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_fixed_array_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_fixed_array_DEPS_TARGET)
            add_library(abseil_absl_fixed_array_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_fixed_array_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_fixed_array_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_fixed_array_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_fixed_array_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_fixed_array_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_fixed_array_LIBS_RELEASE}"
                                      "${abseil_absl_fixed_array_LIB_DIRS_RELEASE}"
                                      abseil_absl_fixed_array_DEPS_TARGET
                                      abseil_absl_fixed_array_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_fixed_array")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::fixed_array
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_fixed_array_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_fixed_array_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_fixed_array_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::fixed_array
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_fixed_array_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::fixed_array PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_fixed_array_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::fixed_array PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_fixed_array_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::fixed_array PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_fixed_array_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::fixed_array PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_fixed_array_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::compressed_tuple #############

        set(abseil_absl_compressed_tuple_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_compressed_tuple_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_compressed_tuple_FRAMEWORKS_RELEASE}" "${abseil_absl_compressed_tuple_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_compressed_tuple_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_compressed_tuple_DEPS_TARGET)
            add_library(abseil_absl_compressed_tuple_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_compressed_tuple_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_compressed_tuple_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_compressed_tuple_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_compressed_tuple_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_compressed_tuple_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_compressed_tuple_LIBS_RELEASE}"
                                      "${abseil_absl_compressed_tuple_LIB_DIRS_RELEASE}"
                                      abseil_absl_compressed_tuple_DEPS_TARGET
                                      abseil_absl_compressed_tuple_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_compressed_tuple")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::compressed_tuple
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_compressed_tuple_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_compressed_tuple_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_compressed_tuple_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::compressed_tuple
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_compressed_tuple_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::compressed_tuple PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_compressed_tuple_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::compressed_tuple PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_compressed_tuple_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::compressed_tuple PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_compressed_tuple_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::compressed_tuple PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_compressed_tuple_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::cleanup #############

        set(abseil_absl_cleanup_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_cleanup_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_cleanup_FRAMEWORKS_RELEASE}" "${abseil_absl_cleanup_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_cleanup_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_cleanup_DEPS_TARGET)
            add_library(abseil_absl_cleanup_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_cleanup_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_cleanup_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_cleanup_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_cleanup_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_cleanup_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_cleanup_LIBS_RELEASE}"
                                      "${abseil_absl_cleanup_LIB_DIRS_RELEASE}"
                                      abseil_absl_cleanup_DEPS_TARGET
                                      abseil_absl_cleanup_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_cleanup")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::cleanup
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_cleanup_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_cleanup_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_cleanup_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::cleanup
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_cleanup_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::cleanup PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_cleanup_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::cleanup PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_cleanup_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::cleanup PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_cleanup_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::cleanup PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_cleanup_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::cleanup_internal #############

        set(abseil_absl_cleanup_internal_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_cleanup_internal_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_cleanup_internal_FRAMEWORKS_RELEASE}" "${abseil_absl_cleanup_internal_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_cleanup_internal_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_cleanup_internal_DEPS_TARGET)
            add_library(abseil_absl_cleanup_internal_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_cleanup_internal_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_cleanup_internal_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_cleanup_internal_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_cleanup_internal_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_cleanup_internal_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_cleanup_internal_LIBS_RELEASE}"
                                      "${abseil_absl_cleanup_internal_LIB_DIRS_RELEASE}"
                                      abseil_absl_cleanup_internal_DEPS_TARGET
                                      abseil_absl_cleanup_internal_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_cleanup_internal")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::cleanup_internal
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_cleanup_internal_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_cleanup_internal_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_cleanup_internal_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::cleanup_internal
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_cleanup_internal_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::cleanup_internal PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_cleanup_internal_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::cleanup_internal PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_cleanup_internal_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::cleanup_internal PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_cleanup_internal_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::cleanup_internal PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_cleanup_internal_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::endian #############

        set(abseil_absl_endian_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_endian_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_endian_FRAMEWORKS_RELEASE}" "${abseil_absl_endian_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_endian_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_endian_DEPS_TARGET)
            add_library(abseil_absl_endian_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_endian_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_endian_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_endian_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_endian_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_endian_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_endian_LIBS_RELEASE}"
                                      "${abseil_absl_endian_LIB_DIRS_RELEASE}"
                                      abseil_absl_endian_DEPS_TARGET
                                      abseil_absl_endian_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_endian")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::endian
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_endian_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_endian_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_endian_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::endian
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_endian_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::endian PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_endian_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::endian PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_endian_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::endian PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_endian_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::endian PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_endian_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::base #############

        set(abseil_absl_base_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_base_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_base_FRAMEWORKS_RELEASE}" "${abseil_absl_base_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_base_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_base_DEPS_TARGET)
            add_library(abseil_absl_base_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_base_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_base_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_base_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_base_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_base_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_base_LIBS_RELEASE}"
                                      "${abseil_absl_base_LIB_DIRS_RELEASE}"
                                      abseil_absl_base_DEPS_TARGET
                                      abseil_absl_base_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_base")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::base
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_base_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_base_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_base_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::base
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_base_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::base PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_base_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::base PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_base_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::base PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_base_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::base PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_base_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::spinlock_wait #############

        set(abseil_absl_spinlock_wait_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_spinlock_wait_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_spinlock_wait_FRAMEWORKS_RELEASE}" "${abseil_absl_spinlock_wait_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_spinlock_wait_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_spinlock_wait_DEPS_TARGET)
            add_library(abseil_absl_spinlock_wait_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_spinlock_wait_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_spinlock_wait_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_spinlock_wait_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_spinlock_wait_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_spinlock_wait_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_spinlock_wait_LIBS_RELEASE}"
                                      "${abseil_absl_spinlock_wait_LIB_DIRS_RELEASE}"
                                      abseil_absl_spinlock_wait_DEPS_TARGET
                                      abseil_absl_spinlock_wait_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_spinlock_wait")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::spinlock_wait
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_spinlock_wait_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_spinlock_wait_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_spinlock_wait_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::spinlock_wait
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_spinlock_wait_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::spinlock_wait PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_spinlock_wait_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::spinlock_wait PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_spinlock_wait_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::spinlock_wait PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_spinlock_wait_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::spinlock_wait PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_spinlock_wait_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::utility #############

        set(abseil_absl_utility_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_utility_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_utility_FRAMEWORKS_RELEASE}" "${abseil_absl_utility_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_utility_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_utility_DEPS_TARGET)
            add_library(abseil_absl_utility_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_utility_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_utility_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_utility_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_utility_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_utility_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_utility_LIBS_RELEASE}"
                                      "${abseil_absl_utility_LIB_DIRS_RELEASE}"
                                      abseil_absl_utility_DEPS_TARGET
                                      abseil_absl_utility_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_utility")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::utility
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_utility_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_utility_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_utility_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::utility
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_utility_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::utility PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_utility_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::utility PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_utility_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::utility PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_utility_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::utility PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_utility_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::bad_variant_access #############

        set(abseil_absl_bad_variant_access_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_bad_variant_access_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_bad_variant_access_FRAMEWORKS_RELEASE}" "${abseil_absl_bad_variant_access_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_bad_variant_access_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_bad_variant_access_DEPS_TARGET)
            add_library(abseil_absl_bad_variant_access_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_bad_variant_access_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_bad_variant_access_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_bad_variant_access_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_bad_variant_access_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_bad_variant_access_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_bad_variant_access_LIBS_RELEASE}"
                                      "${abseil_absl_bad_variant_access_LIB_DIRS_RELEASE}"
                                      abseil_absl_bad_variant_access_DEPS_TARGET
                                      abseil_absl_bad_variant_access_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_bad_variant_access")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::bad_variant_access
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_bad_variant_access_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_bad_variant_access_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_bad_variant_access_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::bad_variant_access
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_bad_variant_access_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::bad_variant_access PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_bad_variant_access_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::bad_variant_access PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_bad_variant_access_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::bad_variant_access PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_bad_variant_access_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::bad_variant_access PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_bad_variant_access_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::bad_optional_access #############

        set(abseil_absl_bad_optional_access_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_bad_optional_access_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_bad_optional_access_FRAMEWORKS_RELEASE}" "${abseil_absl_bad_optional_access_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_bad_optional_access_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_bad_optional_access_DEPS_TARGET)
            add_library(abseil_absl_bad_optional_access_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_bad_optional_access_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_bad_optional_access_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_bad_optional_access_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_bad_optional_access_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_bad_optional_access_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_bad_optional_access_LIBS_RELEASE}"
                                      "${abseil_absl_bad_optional_access_LIB_DIRS_RELEASE}"
                                      abseil_absl_bad_optional_access_DEPS_TARGET
                                      abseil_absl_bad_optional_access_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_bad_optional_access")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::bad_optional_access
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_bad_optional_access_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_bad_optional_access_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_bad_optional_access_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::bad_optional_access
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_bad_optional_access_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::bad_optional_access PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_bad_optional_access_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::bad_optional_access PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_bad_optional_access_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::bad_optional_access PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_bad_optional_access_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::bad_optional_access PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_bad_optional_access_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::span #############

        set(abseil_absl_span_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_span_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_span_FRAMEWORKS_RELEASE}" "${abseil_absl_span_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_span_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_span_DEPS_TARGET)
            add_library(abseil_absl_span_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_span_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_span_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_span_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_span_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_span_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_span_LIBS_RELEASE}"
                                      "${abseil_absl_span_LIB_DIRS_RELEASE}"
                                      abseil_absl_span_DEPS_TARGET
                                      abseil_absl_span_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_span")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::span
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_span_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_span_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_span_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::span
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_span_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::span PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_span_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::span PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_span_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::span PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_span_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::span PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_span_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::bad_any_cast_impl #############

        set(abseil_absl_bad_any_cast_impl_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_bad_any_cast_impl_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_bad_any_cast_impl_FRAMEWORKS_RELEASE}" "${abseil_absl_bad_any_cast_impl_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_bad_any_cast_impl_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_bad_any_cast_impl_DEPS_TARGET)
            add_library(abseil_absl_bad_any_cast_impl_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_bad_any_cast_impl_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_bad_any_cast_impl_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_bad_any_cast_impl_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_bad_any_cast_impl_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_bad_any_cast_impl_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_bad_any_cast_impl_LIBS_RELEASE}"
                                      "${abseil_absl_bad_any_cast_impl_LIB_DIRS_RELEASE}"
                                      abseil_absl_bad_any_cast_impl_DEPS_TARGET
                                      abseil_absl_bad_any_cast_impl_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_bad_any_cast_impl")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::bad_any_cast_impl
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_bad_any_cast_impl_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_bad_any_cast_impl_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_bad_any_cast_impl_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::bad_any_cast_impl
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_bad_any_cast_impl_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::bad_any_cast_impl PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_bad_any_cast_impl_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::bad_any_cast_impl PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_bad_any_cast_impl_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::bad_any_cast_impl PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_bad_any_cast_impl_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::bad_any_cast_impl PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_bad_any_cast_impl_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::cordz_functions #############

        set(abseil_absl_cordz_functions_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_cordz_functions_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_cordz_functions_FRAMEWORKS_RELEASE}" "${abseil_absl_cordz_functions_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_cordz_functions_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_cordz_functions_DEPS_TARGET)
            add_library(abseil_absl_cordz_functions_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_cordz_functions_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_cordz_functions_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_cordz_functions_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_cordz_functions_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_cordz_functions_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_cordz_functions_LIBS_RELEASE}"
                                      "${abseil_absl_cordz_functions_LIB_DIRS_RELEASE}"
                                      abseil_absl_cordz_functions_DEPS_TARGET
                                      abseil_absl_cordz_functions_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_cordz_functions")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::cordz_functions
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_cordz_functions_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_cordz_functions_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_cordz_functions_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::cordz_functions
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_cordz_functions_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::cordz_functions PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_cordz_functions_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::cordz_functions PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_cordz_functions_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::cordz_functions PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_cordz_functions_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::cordz_functions PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_cordz_functions_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::random_internal_randen_hwaes #############

        set(abseil_absl_random_internal_randen_hwaes_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_random_internal_randen_hwaes_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_random_internal_randen_hwaes_FRAMEWORKS_RELEASE}" "${abseil_absl_random_internal_randen_hwaes_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_random_internal_randen_hwaes_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_random_internal_randen_hwaes_DEPS_TARGET)
            add_library(abseil_absl_random_internal_randen_hwaes_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_random_internal_randen_hwaes_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_randen_hwaes_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_randen_hwaes_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_randen_hwaes_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_random_internal_randen_hwaes_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_random_internal_randen_hwaes_LIBS_RELEASE}"
                                      "${abseil_absl_random_internal_randen_hwaes_LIB_DIRS_RELEASE}"
                                      abseil_absl_random_internal_randen_hwaes_DEPS_TARGET
                                      abseil_absl_random_internal_randen_hwaes_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_random_internal_randen_hwaes")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::random_internal_randen_hwaes
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_randen_hwaes_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_randen_hwaes_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_random_internal_randen_hwaes_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::random_internal_randen_hwaes
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_random_internal_randen_hwaes_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::random_internal_randen_hwaes PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_randen_hwaes_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_randen_hwaes PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_randen_hwaes_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_randen_hwaes PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_randen_hwaes_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_randen_hwaes PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_randen_hwaes_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::random_internal_generate_real #############

        set(abseil_absl_random_internal_generate_real_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_random_internal_generate_real_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_random_internal_generate_real_FRAMEWORKS_RELEASE}" "${abseil_absl_random_internal_generate_real_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_random_internal_generate_real_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_random_internal_generate_real_DEPS_TARGET)
            add_library(abseil_absl_random_internal_generate_real_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_random_internal_generate_real_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_generate_real_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_generate_real_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_generate_real_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_random_internal_generate_real_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_random_internal_generate_real_LIBS_RELEASE}"
                                      "${abseil_absl_random_internal_generate_real_LIB_DIRS_RELEASE}"
                                      abseil_absl_random_internal_generate_real_DEPS_TARGET
                                      abseil_absl_random_internal_generate_real_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_random_internal_generate_real")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::random_internal_generate_real
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_generate_real_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_generate_real_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_random_internal_generate_real_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::random_internal_generate_real
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_random_internal_generate_real_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::random_internal_generate_real PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_generate_real_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_generate_real PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_generate_real_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_generate_real PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_generate_real_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_generate_real PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_generate_real_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::memory #############

        set(abseil_absl_memory_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_memory_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_memory_FRAMEWORKS_RELEASE}" "${abseil_absl_memory_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_memory_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_memory_DEPS_TARGET)
            add_library(abseil_absl_memory_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_memory_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_memory_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_memory_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_memory_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_memory_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_memory_LIBS_RELEASE}"
                                      "${abseil_absl_memory_LIB_DIRS_RELEASE}"
                                      abseil_absl_memory_DEPS_TARGET
                                      abseil_absl_memory_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_memory")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::memory
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_memory_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_memory_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_memory_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::memory
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_memory_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::memory PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_memory_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::memory PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_memory_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::memory PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_memory_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::memory PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_memory_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::function_ref #############

        set(abseil_absl_function_ref_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_function_ref_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_function_ref_FRAMEWORKS_RELEASE}" "${abseil_absl_function_ref_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_function_ref_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_function_ref_DEPS_TARGET)
            add_library(abseil_absl_function_ref_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_function_ref_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_function_ref_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_function_ref_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_function_ref_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_function_ref_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_function_ref_LIBS_RELEASE}"
                                      "${abseil_absl_function_ref_LIB_DIRS_RELEASE}"
                                      abseil_absl_function_ref_DEPS_TARGET
                                      abseil_absl_function_ref_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_function_ref")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::function_ref
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_function_ref_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_function_ref_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_function_ref_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::function_ref
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_function_ref_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::function_ref PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_function_ref_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::function_ref PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_function_ref_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::function_ref PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_function_ref_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::function_ref PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_function_ref_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::debugging_internal #############

        set(abseil_absl_debugging_internal_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_debugging_internal_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_debugging_internal_FRAMEWORKS_RELEASE}" "${abseil_absl_debugging_internal_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_debugging_internal_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_debugging_internal_DEPS_TARGET)
            add_library(abseil_absl_debugging_internal_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_debugging_internal_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_debugging_internal_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_debugging_internal_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_debugging_internal_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_debugging_internal_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_debugging_internal_LIBS_RELEASE}"
                                      "${abseil_absl_debugging_internal_LIB_DIRS_RELEASE}"
                                      abseil_absl_debugging_internal_DEPS_TARGET
                                      abseil_absl_debugging_internal_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_debugging_internal")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::debugging_internal
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_debugging_internal_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_debugging_internal_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_debugging_internal_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::debugging_internal
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_debugging_internal_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::debugging_internal PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_debugging_internal_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::debugging_internal PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_debugging_internal_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::debugging_internal PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_debugging_internal_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::debugging_internal PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_debugging_internal_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::container_common #############

        set(abseil_absl_container_common_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_container_common_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_container_common_FRAMEWORKS_RELEASE}" "${abseil_absl_container_common_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_container_common_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_container_common_DEPS_TARGET)
            add_library(abseil_absl_container_common_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_container_common_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_container_common_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_container_common_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_container_common_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_container_common_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_container_common_LIBS_RELEASE}"
                                      "${abseil_absl_container_common_LIB_DIRS_RELEASE}"
                                      abseil_absl_container_common_DEPS_TARGET
                                      abseil_absl_container_common_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_container_common")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::container_common
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_container_common_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_container_common_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_container_common_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::container_common
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_container_common_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::container_common PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_container_common_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::container_common PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_container_common_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::container_common PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_container_common_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::container_common PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_container_common_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::hashtable_debug #############

        set(abseil_absl_hashtable_debug_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_hashtable_debug_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_hashtable_debug_FRAMEWORKS_RELEASE}" "${abseil_absl_hashtable_debug_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_hashtable_debug_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_hashtable_debug_DEPS_TARGET)
            add_library(abseil_absl_hashtable_debug_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_hashtable_debug_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_hashtable_debug_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_hashtable_debug_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_hashtable_debug_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_hashtable_debug_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_hashtable_debug_LIBS_RELEASE}"
                                      "${abseil_absl_hashtable_debug_LIB_DIRS_RELEASE}"
                                      abseil_absl_hashtable_debug_DEPS_TARGET
                                      abseil_absl_hashtable_debug_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_hashtable_debug")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::hashtable_debug
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_hashtable_debug_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_hashtable_debug_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_hashtable_debug_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::hashtable_debug
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_hashtable_debug_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::hashtable_debug PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_hashtable_debug_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::hashtable_debug PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_hashtable_debug_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::hashtable_debug PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_hashtable_debug_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::hashtable_debug PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_hashtable_debug_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::hash_policy_traits #############

        set(abseil_absl_hash_policy_traits_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_hash_policy_traits_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_hash_policy_traits_FRAMEWORKS_RELEASE}" "${abseil_absl_hash_policy_traits_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_hash_policy_traits_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_hash_policy_traits_DEPS_TARGET)
            add_library(abseil_absl_hash_policy_traits_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_hash_policy_traits_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_hash_policy_traits_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_hash_policy_traits_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_hash_policy_traits_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_hash_policy_traits_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_hash_policy_traits_LIBS_RELEASE}"
                                      "${abseil_absl_hash_policy_traits_LIB_DIRS_RELEASE}"
                                      abseil_absl_hash_policy_traits_DEPS_TARGET
                                      abseil_absl_hash_policy_traits_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_hash_policy_traits")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::hash_policy_traits
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_hash_policy_traits_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_hash_policy_traits_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_hash_policy_traits_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::hash_policy_traits
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_hash_policy_traits_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::hash_policy_traits PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_hash_policy_traits_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::hash_policy_traits PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_hash_policy_traits_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::hash_policy_traits PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_hash_policy_traits_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::hash_policy_traits PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_hash_policy_traits_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::algorithm_container #############

        set(abseil_absl_algorithm_container_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_algorithm_container_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_algorithm_container_FRAMEWORKS_RELEASE}" "${abseil_absl_algorithm_container_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_algorithm_container_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_algorithm_container_DEPS_TARGET)
            add_library(abseil_absl_algorithm_container_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_algorithm_container_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_algorithm_container_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_algorithm_container_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_algorithm_container_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_algorithm_container_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_algorithm_container_LIBS_RELEASE}"
                                      "${abseil_absl_algorithm_container_LIB_DIRS_RELEASE}"
                                      abseil_absl_algorithm_container_DEPS_TARGET
                                      abseil_absl_algorithm_container_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_algorithm_container")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::algorithm_container
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_algorithm_container_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_algorithm_container_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_algorithm_container_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::algorithm_container
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_algorithm_container_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::algorithm_container PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_algorithm_container_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::algorithm_container PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_algorithm_container_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::algorithm_container PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_algorithm_container_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::algorithm_container PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_algorithm_container_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::strerror #############

        set(abseil_absl_strerror_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_strerror_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_strerror_FRAMEWORKS_RELEASE}" "${abseil_absl_strerror_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_strerror_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_strerror_DEPS_TARGET)
            add_library(abseil_absl_strerror_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_strerror_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_strerror_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_strerror_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_strerror_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_strerror_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_strerror_LIBS_RELEASE}"
                                      "${abseil_absl_strerror_LIB_DIRS_RELEASE}"
                                      abseil_absl_strerror_DEPS_TARGET
                                      abseil_absl_strerror_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_strerror")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::strerror
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_strerror_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_strerror_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_strerror_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::strerror
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_strerror_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::strerror PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_strerror_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::strerror PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_strerror_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::strerror PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_strerror_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::strerror PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_strerror_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::scoped_set_env #############

        set(abseil_absl_scoped_set_env_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_scoped_set_env_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_scoped_set_env_FRAMEWORKS_RELEASE}" "${abseil_absl_scoped_set_env_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_scoped_set_env_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_scoped_set_env_DEPS_TARGET)
            add_library(abseil_absl_scoped_set_env_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_scoped_set_env_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_scoped_set_env_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_scoped_set_env_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_scoped_set_env_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_scoped_set_env_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_scoped_set_env_LIBS_RELEASE}"
                                      "${abseil_absl_scoped_set_env_LIB_DIRS_RELEASE}"
                                      abseil_absl_scoped_set_env_DEPS_TARGET
                                      abseil_absl_scoped_set_env_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_scoped_set_env")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::scoped_set_env
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_scoped_set_env_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_scoped_set_env_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_scoped_set_env_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::scoped_set_env
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_scoped_set_env_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::scoped_set_env PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_scoped_set_env_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::scoped_set_env PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_scoped_set_env_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::scoped_set_env PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_scoped_set_env_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::scoped_set_env PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_scoped_set_env_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::throw_delegate #############

        set(abseil_absl_throw_delegate_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_throw_delegate_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_throw_delegate_FRAMEWORKS_RELEASE}" "${abseil_absl_throw_delegate_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_throw_delegate_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_throw_delegate_DEPS_TARGET)
            add_library(abseil_absl_throw_delegate_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_throw_delegate_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_throw_delegate_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_throw_delegate_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_throw_delegate_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_throw_delegate_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_throw_delegate_LIBS_RELEASE}"
                                      "${abseil_absl_throw_delegate_LIB_DIRS_RELEASE}"
                                      abseil_absl_throw_delegate_DEPS_TARGET
                                      abseil_absl_throw_delegate_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_throw_delegate")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::throw_delegate
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_throw_delegate_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_throw_delegate_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_throw_delegate_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::throw_delegate
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_throw_delegate_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::throw_delegate PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_throw_delegate_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::throw_delegate PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_throw_delegate_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::throw_delegate PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_throw_delegate_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::throw_delegate PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_throw_delegate_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::base_internal #############

        set(abseil_absl_base_internal_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_base_internal_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_base_internal_FRAMEWORKS_RELEASE}" "${abseil_absl_base_internal_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_base_internal_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_base_internal_DEPS_TARGET)
            add_library(abseil_absl_base_internal_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_base_internal_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_base_internal_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_base_internal_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_base_internal_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_base_internal_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_base_internal_LIBS_RELEASE}"
                                      "${abseil_absl_base_internal_LIB_DIRS_RELEASE}"
                                      abseil_absl_base_internal_DEPS_TARGET
                                      abseil_absl_base_internal_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_base_internal")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::base_internal
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_base_internal_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_base_internal_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_base_internal_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::base_internal
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_base_internal_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::base_internal PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_base_internal_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::base_internal PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_base_internal_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::base_internal PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_base_internal_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::base_internal PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_base_internal_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::raw_logging_internal #############

        set(abseil_absl_raw_logging_internal_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_raw_logging_internal_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_raw_logging_internal_FRAMEWORKS_RELEASE}" "${abseil_absl_raw_logging_internal_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_raw_logging_internal_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_raw_logging_internal_DEPS_TARGET)
            add_library(abseil_absl_raw_logging_internal_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_raw_logging_internal_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_raw_logging_internal_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_raw_logging_internal_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_raw_logging_internal_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_raw_logging_internal_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_raw_logging_internal_LIBS_RELEASE}"
                                      "${abseil_absl_raw_logging_internal_LIB_DIRS_RELEASE}"
                                      abseil_absl_raw_logging_internal_DEPS_TARGET
                                      abseil_absl_raw_logging_internal_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_raw_logging_internal")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::raw_logging_internal
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_raw_logging_internal_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_raw_logging_internal_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_raw_logging_internal_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::raw_logging_internal
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_raw_logging_internal_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::raw_logging_internal PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_raw_logging_internal_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::raw_logging_internal PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_raw_logging_internal_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::raw_logging_internal PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_raw_logging_internal_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::raw_logging_internal PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_raw_logging_internal_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::log_severity #############

        set(abseil_absl_log_severity_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_log_severity_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_log_severity_FRAMEWORKS_RELEASE}" "${abseil_absl_log_severity_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_log_severity_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_log_severity_DEPS_TARGET)
            add_library(abseil_absl_log_severity_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_log_severity_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_log_severity_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_log_severity_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_log_severity_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_log_severity_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_log_severity_LIBS_RELEASE}"
                                      "${abseil_absl_log_severity_LIB_DIRS_RELEASE}"
                                      abseil_absl_log_severity_DEPS_TARGET
                                      abseil_absl_log_severity_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_log_severity")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::log_severity
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_log_severity_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_log_severity_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_log_severity_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::log_severity
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_log_severity_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::log_severity PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_log_severity_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::log_severity PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_log_severity_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::log_severity PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_log_severity_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::log_severity PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_log_severity_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::errno_saver #############

        set(abseil_absl_errno_saver_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_errno_saver_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_errno_saver_FRAMEWORKS_RELEASE}" "${abseil_absl_errno_saver_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_errno_saver_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_errno_saver_DEPS_TARGET)
            add_library(abseil_absl_errno_saver_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_errno_saver_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_errno_saver_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_errno_saver_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_errno_saver_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_errno_saver_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_errno_saver_LIBS_RELEASE}"
                                      "${abseil_absl_errno_saver_LIB_DIRS_RELEASE}"
                                      abseil_absl_errno_saver_DEPS_TARGET
                                      abseil_absl_errno_saver_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_errno_saver")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::errno_saver
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_errno_saver_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_errno_saver_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_errno_saver_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::errno_saver
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_errno_saver_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::errno_saver PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_errno_saver_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::errno_saver PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_errno_saver_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::errno_saver PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_errno_saver_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::errno_saver PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_errno_saver_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::atomic_hook #############

        set(abseil_absl_atomic_hook_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_atomic_hook_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_atomic_hook_FRAMEWORKS_RELEASE}" "${abseil_absl_atomic_hook_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_atomic_hook_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_atomic_hook_DEPS_TARGET)
            add_library(abseil_absl_atomic_hook_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_atomic_hook_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_atomic_hook_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_atomic_hook_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_atomic_hook_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_atomic_hook_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_atomic_hook_LIBS_RELEASE}"
                                      "${abseil_absl_atomic_hook_LIB_DIRS_RELEASE}"
                                      abseil_absl_atomic_hook_DEPS_TARGET
                                      abseil_absl_atomic_hook_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_atomic_hook")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::atomic_hook
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_atomic_hook_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_atomic_hook_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_atomic_hook_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::atomic_hook
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_atomic_hook_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::atomic_hook PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_atomic_hook_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::atomic_hook PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_atomic_hook_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::atomic_hook PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_atomic_hook_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::atomic_hook PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_atomic_hook_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::compare #############

        set(abseil_absl_compare_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_compare_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_compare_FRAMEWORKS_RELEASE}" "${abseil_absl_compare_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_compare_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_compare_DEPS_TARGET)
            add_library(abseil_absl_compare_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_compare_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_compare_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_compare_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_compare_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_compare_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_compare_LIBS_RELEASE}"
                                      "${abseil_absl_compare_LIB_DIRS_RELEASE}"
                                      abseil_absl_compare_DEPS_TARGET
                                      abseil_absl_compare_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_compare")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::compare
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_compare_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_compare_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_compare_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::compare
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_compare_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::compare PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_compare_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::compare PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_compare_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::compare PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_compare_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::compare PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_compare_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::time_zone #############

        set(abseil_absl_time_zone_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_time_zone_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_time_zone_FRAMEWORKS_RELEASE}" "${abseil_absl_time_zone_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_time_zone_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_time_zone_DEPS_TARGET)
            add_library(abseil_absl_time_zone_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_time_zone_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_time_zone_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_time_zone_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_time_zone_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_time_zone_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_time_zone_LIBS_RELEASE}"
                                      "${abseil_absl_time_zone_LIB_DIRS_RELEASE}"
                                      abseil_absl_time_zone_DEPS_TARGET
                                      abseil_absl_time_zone_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_time_zone")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::time_zone
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_time_zone_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_time_zone_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_time_zone_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::time_zone
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_time_zone_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::time_zone PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_time_zone_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::time_zone PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_time_zone_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::time_zone PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_time_zone_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::time_zone PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_time_zone_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::civil_time #############

        set(abseil_absl_civil_time_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_civil_time_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_civil_time_FRAMEWORKS_RELEASE}" "${abseil_absl_civil_time_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_civil_time_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_civil_time_DEPS_TARGET)
            add_library(abseil_absl_civil_time_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_civil_time_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_civil_time_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_civil_time_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_civil_time_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_civil_time_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_civil_time_LIBS_RELEASE}"
                                      "${abseil_absl_civil_time_LIB_DIRS_RELEASE}"
                                      abseil_absl_civil_time_DEPS_TARGET
                                      abseil_absl_civil_time_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_civil_time")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::civil_time
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_civil_time_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_civil_time_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_civil_time_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::civil_time
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_civil_time_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::civil_time PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_civil_time_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::civil_time PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_civil_time_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::civil_time PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_civil_time_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::civil_time PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_civil_time_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::cordz_update_tracker #############

        set(abseil_absl_cordz_update_tracker_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_cordz_update_tracker_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_cordz_update_tracker_FRAMEWORKS_RELEASE}" "${abseil_absl_cordz_update_tracker_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_cordz_update_tracker_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_cordz_update_tracker_DEPS_TARGET)
            add_library(abseil_absl_cordz_update_tracker_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_cordz_update_tracker_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_cordz_update_tracker_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_cordz_update_tracker_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_cordz_update_tracker_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_cordz_update_tracker_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_cordz_update_tracker_LIBS_RELEASE}"
                                      "${abseil_absl_cordz_update_tracker_LIB_DIRS_RELEASE}"
                                      abseil_absl_cordz_update_tracker_DEPS_TARGET
                                      abseil_absl_cordz_update_tracker_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_cordz_update_tracker")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::cordz_update_tracker
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_cordz_update_tracker_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_cordz_update_tracker_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_cordz_update_tracker_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::cordz_update_tracker
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_cordz_update_tracker_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::cordz_update_tracker PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_cordz_update_tracker_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::cordz_update_tracker PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_cordz_update_tracker_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::cordz_update_tracker PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_cordz_update_tracker_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::cordz_update_tracker PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_cordz_update_tracker_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::random_internal_uniform_helper #############

        set(abseil_absl_random_internal_uniform_helper_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_random_internal_uniform_helper_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_random_internal_uniform_helper_FRAMEWORKS_RELEASE}" "${abseil_absl_random_internal_uniform_helper_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_random_internal_uniform_helper_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_random_internal_uniform_helper_DEPS_TARGET)
            add_library(abseil_absl_random_internal_uniform_helper_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_random_internal_uniform_helper_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_uniform_helper_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_uniform_helper_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_uniform_helper_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_random_internal_uniform_helper_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_random_internal_uniform_helper_LIBS_RELEASE}"
                                      "${abseil_absl_random_internal_uniform_helper_LIB_DIRS_RELEASE}"
                                      abseil_absl_random_internal_uniform_helper_DEPS_TARGET
                                      abseil_absl_random_internal_uniform_helper_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_random_internal_uniform_helper")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::random_internal_uniform_helper
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_uniform_helper_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_uniform_helper_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_random_internal_uniform_helper_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::random_internal_uniform_helper
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_random_internal_uniform_helper_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::random_internal_uniform_helper PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_uniform_helper_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_uniform_helper PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_uniform_helper_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_uniform_helper PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_uniform_helper_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_uniform_helper PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_uniform_helper_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::random_internal_randen_hwaes_impl #############

        set(abseil_absl_random_internal_randen_hwaes_impl_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_random_internal_randen_hwaes_impl_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_random_internal_randen_hwaes_impl_FRAMEWORKS_RELEASE}" "${abseil_absl_random_internal_randen_hwaes_impl_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_random_internal_randen_hwaes_impl_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_random_internal_randen_hwaes_impl_DEPS_TARGET)
            add_library(abseil_absl_random_internal_randen_hwaes_impl_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_random_internal_randen_hwaes_impl_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_randen_hwaes_impl_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_randen_hwaes_impl_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_randen_hwaes_impl_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_random_internal_randen_hwaes_impl_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_random_internal_randen_hwaes_impl_LIBS_RELEASE}"
                                      "${abseil_absl_random_internal_randen_hwaes_impl_LIB_DIRS_RELEASE}"
                                      abseil_absl_random_internal_randen_hwaes_impl_DEPS_TARGET
                                      abseil_absl_random_internal_randen_hwaes_impl_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_random_internal_randen_hwaes_impl")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::random_internal_randen_hwaes_impl
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_randen_hwaes_impl_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_randen_hwaes_impl_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_random_internal_randen_hwaes_impl_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::random_internal_randen_hwaes_impl
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_random_internal_randen_hwaes_impl_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::random_internal_randen_hwaes_impl PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_randen_hwaes_impl_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_randen_hwaes_impl PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_randen_hwaes_impl_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_randen_hwaes_impl PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_randen_hwaes_impl_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_randen_hwaes_impl PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_randen_hwaes_impl_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::random_internal_randen_slow #############

        set(abseil_absl_random_internal_randen_slow_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_random_internal_randen_slow_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_random_internal_randen_slow_FRAMEWORKS_RELEASE}" "${abseil_absl_random_internal_randen_slow_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_random_internal_randen_slow_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_random_internal_randen_slow_DEPS_TARGET)
            add_library(abseil_absl_random_internal_randen_slow_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_random_internal_randen_slow_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_randen_slow_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_randen_slow_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_randen_slow_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_random_internal_randen_slow_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_random_internal_randen_slow_LIBS_RELEASE}"
                                      "${abseil_absl_random_internal_randen_slow_LIB_DIRS_RELEASE}"
                                      abseil_absl_random_internal_randen_slow_DEPS_TARGET
                                      abseil_absl_random_internal_randen_slow_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_random_internal_randen_slow")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::random_internal_randen_slow
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_randen_slow_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_randen_slow_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_random_internal_randen_slow_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::random_internal_randen_slow
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_random_internal_randen_slow_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::random_internal_randen_slow PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_randen_slow_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_randen_slow PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_randen_slow_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_randen_slow PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_randen_slow_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_randen_slow PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_randen_slow_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::random_internal_platform #############

        set(abseil_absl_random_internal_platform_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_random_internal_platform_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_random_internal_platform_FRAMEWORKS_RELEASE}" "${abseil_absl_random_internal_platform_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_random_internal_platform_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_random_internal_platform_DEPS_TARGET)
            add_library(abseil_absl_random_internal_platform_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_random_internal_platform_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_platform_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_platform_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_platform_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_random_internal_platform_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_random_internal_platform_LIBS_RELEASE}"
                                      "${abseil_absl_random_internal_platform_LIB_DIRS_RELEASE}"
                                      abseil_absl_random_internal_platform_DEPS_TARGET
                                      abseil_absl_random_internal_platform_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_random_internal_platform")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::random_internal_platform
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_platform_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_platform_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_random_internal_platform_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::random_internal_platform
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_random_internal_platform_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::random_internal_platform PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_platform_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_platform PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_platform_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_platform PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_platform_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_platform PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_platform_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::random_internal_pcg_engine #############

        set(abseil_absl_random_internal_pcg_engine_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_random_internal_pcg_engine_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_random_internal_pcg_engine_FRAMEWORKS_RELEASE}" "${abseil_absl_random_internal_pcg_engine_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_random_internal_pcg_engine_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_random_internal_pcg_engine_DEPS_TARGET)
            add_library(abseil_absl_random_internal_pcg_engine_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_random_internal_pcg_engine_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_pcg_engine_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_pcg_engine_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_pcg_engine_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_random_internal_pcg_engine_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_random_internal_pcg_engine_LIBS_RELEASE}"
                                      "${abseil_absl_random_internal_pcg_engine_LIB_DIRS_RELEASE}"
                                      abseil_absl_random_internal_pcg_engine_DEPS_TARGET
                                      abseil_absl_random_internal_pcg_engine_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_random_internal_pcg_engine")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::random_internal_pcg_engine
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_pcg_engine_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_pcg_engine_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_random_internal_pcg_engine_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::random_internal_pcg_engine
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_random_internal_pcg_engine_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::random_internal_pcg_engine PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_pcg_engine_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_pcg_engine PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_pcg_engine_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_pcg_engine PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_pcg_engine_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_pcg_engine PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_pcg_engine_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::random_internal_fastmath #############

        set(abseil_absl_random_internal_fastmath_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_random_internal_fastmath_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_random_internal_fastmath_FRAMEWORKS_RELEASE}" "${abseil_absl_random_internal_fastmath_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_random_internal_fastmath_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_random_internal_fastmath_DEPS_TARGET)
            add_library(abseil_absl_random_internal_fastmath_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_random_internal_fastmath_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_fastmath_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_fastmath_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_fastmath_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_random_internal_fastmath_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_random_internal_fastmath_LIBS_RELEASE}"
                                      "${abseil_absl_random_internal_fastmath_LIB_DIRS_RELEASE}"
                                      abseil_absl_random_internal_fastmath_DEPS_TARGET
                                      abseil_absl_random_internal_fastmath_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_random_internal_fastmath")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::random_internal_fastmath
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_fastmath_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_fastmath_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_random_internal_fastmath_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::random_internal_fastmath
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_random_internal_fastmath_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::random_internal_fastmath PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_fastmath_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_fastmath PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_fastmath_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_fastmath PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_fastmath_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_fastmath PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_fastmath_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::random_internal_wide_multiply #############

        set(abseil_absl_random_internal_wide_multiply_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_random_internal_wide_multiply_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_random_internal_wide_multiply_FRAMEWORKS_RELEASE}" "${abseil_absl_random_internal_wide_multiply_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_random_internal_wide_multiply_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_random_internal_wide_multiply_DEPS_TARGET)
            add_library(abseil_absl_random_internal_wide_multiply_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_random_internal_wide_multiply_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_wide_multiply_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_wide_multiply_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_wide_multiply_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_random_internal_wide_multiply_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_random_internal_wide_multiply_LIBS_RELEASE}"
                                      "${abseil_absl_random_internal_wide_multiply_LIB_DIRS_RELEASE}"
                                      abseil_absl_random_internal_wide_multiply_DEPS_TARGET
                                      abseil_absl_random_internal_wide_multiply_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_random_internal_wide_multiply")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::random_internal_wide_multiply
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_wide_multiply_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_wide_multiply_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_random_internal_wide_multiply_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::random_internal_wide_multiply
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_random_internal_wide_multiply_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::random_internal_wide_multiply PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_wide_multiply_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_wide_multiply PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_wide_multiply_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_wide_multiply PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_wide_multiply_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_wide_multiply PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_wide_multiply_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::random_internal_iostream_state_saver #############

        set(abseil_absl_random_internal_iostream_state_saver_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_random_internal_iostream_state_saver_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_random_internal_iostream_state_saver_FRAMEWORKS_RELEASE}" "${abseil_absl_random_internal_iostream_state_saver_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_random_internal_iostream_state_saver_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_random_internal_iostream_state_saver_DEPS_TARGET)
            add_library(abseil_absl_random_internal_iostream_state_saver_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_random_internal_iostream_state_saver_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_iostream_state_saver_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_iostream_state_saver_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_iostream_state_saver_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_random_internal_iostream_state_saver_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_random_internal_iostream_state_saver_LIBS_RELEASE}"
                                      "${abseil_absl_random_internal_iostream_state_saver_LIB_DIRS_RELEASE}"
                                      abseil_absl_random_internal_iostream_state_saver_DEPS_TARGET
                                      abseil_absl_random_internal_iostream_state_saver_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_random_internal_iostream_state_saver")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::random_internal_iostream_state_saver
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_iostream_state_saver_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_iostream_state_saver_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_random_internal_iostream_state_saver_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::random_internal_iostream_state_saver
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_random_internal_iostream_state_saver_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::random_internal_iostream_state_saver PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_iostream_state_saver_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_iostream_state_saver PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_iostream_state_saver_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_iostream_state_saver PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_iostream_state_saver_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_iostream_state_saver PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_iostream_state_saver_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::random_internal_fast_uniform_bits #############

        set(abseil_absl_random_internal_fast_uniform_bits_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_random_internal_fast_uniform_bits_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_random_internal_fast_uniform_bits_FRAMEWORKS_RELEASE}" "${abseil_absl_random_internal_fast_uniform_bits_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_random_internal_fast_uniform_bits_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_random_internal_fast_uniform_bits_DEPS_TARGET)
            add_library(abseil_absl_random_internal_fast_uniform_bits_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_random_internal_fast_uniform_bits_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_fast_uniform_bits_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_fast_uniform_bits_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_fast_uniform_bits_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_random_internal_fast_uniform_bits_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_random_internal_fast_uniform_bits_LIBS_RELEASE}"
                                      "${abseil_absl_random_internal_fast_uniform_bits_LIB_DIRS_RELEASE}"
                                      abseil_absl_random_internal_fast_uniform_bits_DEPS_TARGET
                                      abseil_absl_random_internal_fast_uniform_bits_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_random_internal_fast_uniform_bits")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::random_internal_fast_uniform_bits
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_fast_uniform_bits_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_fast_uniform_bits_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_random_internal_fast_uniform_bits_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::random_internal_fast_uniform_bits
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_random_internal_fast_uniform_bits_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::random_internal_fast_uniform_bits PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_fast_uniform_bits_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_fast_uniform_bits PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_fast_uniform_bits_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_fast_uniform_bits PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_fast_uniform_bits_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_fast_uniform_bits PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_fast_uniform_bits_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::random_internal_traits #############

        set(abseil_absl_random_internal_traits_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_random_internal_traits_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_random_internal_traits_FRAMEWORKS_RELEASE}" "${abseil_absl_random_internal_traits_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_random_internal_traits_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_random_internal_traits_DEPS_TARGET)
            add_library(abseil_absl_random_internal_traits_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_random_internal_traits_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_traits_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_traits_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_traits_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_random_internal_traits_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_random_internal_traits_LIBS_RELEASE}"
                                      "${abseil_absl_random_internal_traits_LIB_DIRS_RELEASE}"
                                      abseil_absl_random_internal_traits_DEPS_TARGET
                                      abseil_absl_random_internal_traits_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_random_internal_traits")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::random_internal_traits
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_traits_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_traits_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_random_internal_traits_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::random_internal_traits
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_random_internal_traits_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::random_internal_traits PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_traits_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_traits PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_traits_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_traits PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_traits_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::random_internal_traits PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_internal_traits_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::random_seed_gen_exception #############

        set(abseil_absl_random_seed_gen_exception_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_random_seed_gen_exception_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_random_seed_gen_exception_FRAMEWORKS_RELEASE}" "${abseil_absl_random_seed_gen_exception_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_random_seed_gen_exception_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_random_seed_gen_exception_DEPS_TARGET)
            add_library(abseil_absl_random_seed_gen_exception_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_random_seed_gen_exception_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_random_seed_gen_exception_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_seed_gen_exception_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_seed_gen_exception_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_random_seed_gen_exception_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_random_seed_gen_exception_LIBS_RELEASE}"
                                      "${abseil_absl_random_seed_gen_exception_LIB_DIRS_RELEASE}"
                                      abseil_absl_random_seed_gen_exception_DEPS_TARGET
                                      abseil_absl_random_seed_gen_exception_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_random_seed_gen_exception")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::random_seed_gen_exception
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_random_seed_gen_exception_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_random_seed_gen_exception_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_random_seed_gen_exception_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::random_seed_gen_exception
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_random_seed_gen_exception_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::random_seed_gen_exception PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_seed_gen_exception_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::random_seed_gen_exception PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_random_seed_gen_exception_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::random_seed_gen_exception PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_seed_gen_exception_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::random_seed_gen_exception PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_random_seed_gen_exception_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::periodic_sampler #############

        set(abseil_absl_periodic_sampler_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_periodic_sampler_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_periodic_sampler_FRAMEWORKS_RELEASE}" "${abseil_absl_periodic_sampler_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_periodic_sampler_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_periodic_sampler_DEPS_TARGET)
            add_library(abseil_absl_periodic_sampler_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_periodic_sampler_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_periodic_sampler_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_periodic_sampler_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_periodic_sampler_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_periodic_sampler_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_periodic_sampler_LIBS_RELEASE}"
                                      "${abseil_absl_periodic_sampler_LIB_DIRS_RELEASE}"
                                      abseil_absl_periodic_sampler_DEPS_TARGET
                                      abseil_absl_periodic_sampler_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_periodic_sampler")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::periodic_sampler
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_periodic_sampler_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_periodic_sampler_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_periodic_sampler_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::periodic_sampler
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_periodic_sampler_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::periodic_sampler PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_periodic_sampler_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::periodic_sampler PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_periodic_sampler_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::periodic_sampler PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_periodic_sampler_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::periodic_sampler PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_periodic_sampler_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::exponential_biased #############

        set(abseil_absl_exponential_biased_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_exponential_biased_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_exponential_biased_FRAMEWORKS_RELEASE}" "${abseil_absl_exponential_biased_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_exponential_biased_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_exponential_biased_DEPS_TARGET)
            add_library(abseil_absl_exponential_biased_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_exponential_biased_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_exponential_biased_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_exponential_biased_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_exponential_biased_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_exponential_biased_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_exponential_biased_LIBS_RELEASE}"
                                      "${abseil_absl_exponential_biased_LIB_DIRS_RELEASE}"
                                      abseil_absl_exponential_biased_DEPS_TARGET
                                      abseil_absl_exponential_biased_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_exponential_biased")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::exponential_biased
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_exponential_biased_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_exponential_biased_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_exponential_biased_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::exponential_biased
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_exponential_biased_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::exponential_biased PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_exponential_biased_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::exponential_biased PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_exponential_biased_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::exponential_biased PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_exponential_biased_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::exponential_biased PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_exponential_biased_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::numeric_representation #############

        set(abseil_absl_numeric_representation_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_numeric_representation_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_numeric_representation_FRAMEWORKS_RELEASE}" "${abseil_absl_numeric_representation_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_numeric_representation_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_numeric_representation_DEPS_TARGET)
            add_library(abseil_absl_numeric_representation_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_numeric_representation_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_numeric_representation_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_numeric_representation_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_numeric_representation_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_numeric_representation_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_numeric_representation_LIBS_RELEASE}"
                                      "${abseil_absl_numeric_representation_LIB_DIRS_RELEASE}"
                                      abseil_absl_numeric_representation_DEPS_TARGET
                                      abseil_absl_numeric_representation_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_numeric_representation")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::numeric_representation
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_numeric_representation_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_numeric_representation_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_numeric_representation_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::numeric_representation
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_numeric_representation_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::numeric_representation PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_numeric_representation_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::numeric_representation PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_numeric_representation_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::numeric_representation PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_numeric_representation_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::numeric_representation PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_numeric_representation_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::numeric #############

        set(abseil_absl_numeric_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_numeric_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_numeric_FRAMEWORKS_RELEASE}" "${abseil_absl_numeric_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_numeric_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_numeric_DEPS_TARGET)
            add_library(abseil_absl_numeric_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_numeric_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_numeric_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_numeric_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_numeric_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_numeric_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_numeric_LIBS_RELEASE}"
                                      "${abseil_absl_numeric_LIB_DIRS_RELEASE}"
                                      abseil_absl_numeric_DEPS_TARGET
                                      abseil_absl_numeric_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_numeric")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::numeric
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_numeric_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_numeric_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_numeric_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::numeric
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_numeric_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::numeric PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_numeric_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::numeric PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_numeric_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::numeric PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_numeric_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::numeric PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_numeric_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::int128 #############

        set(abseil_absl_int128_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_int128_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_int128_FRAMEWORKS_RELEASE}" "${abseil_absl_int128_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_int128_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_int128_DEPS_TARGET)
            add_library(abseil_absl_int128_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_int128_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_int128_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_int128_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_int128_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_int128_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_int128_LIBS_RELEASE}"
                                      "${abseil_absl_int128_LIB_DIRS_RELEASE}"
                                      abseil_absl_int128_DEPS_TARGET
                                      abseil_absl_int128_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_int128")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::int128
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_int128_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_int128_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_int128_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::int128
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_int128_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::int128 PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_int128_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::int128 PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_int128_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::int128 PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_int128_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::int128 PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_int128_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::bits #############

        set(abseil_absl_bits_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_bits_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_bits_FRAMEWORKS_RELEASE}" "${abseil_absl_bits_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_bits_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_bits_DEPS_TARGET)
            add_library(abseil_absl_bits_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_bits_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_bits_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_bits_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_bits_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_bits_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_bits_LIBS_RELEASE}"
                                      "${abseil_absl_bits_LIB_DIRS_RELEASE}"
                                      abseil_absl_bits_DEPS_TARGET
                                      abseil_absl_bits_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_bits")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::bits
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_bits_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_bits_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_bits_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::bits
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_bits_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::bits PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_bits_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::bits PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_bits_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::bits PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_bits_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::bits PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_bits_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::meta #############

        set(abseil_absl_meta_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_meta_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_meta_FRAMEWORKS_RELEASE}" "${abseil_absl_meta_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_meta_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_meta_DEPS_TARGET)
            add_library(abseil_absl_meta_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_meta_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_meta_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_meta_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_meta_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_meta_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_meta_LIBS_RELEASE}"
                                      "${abseil_absl_meta_LIB_DIRS_RELEASE}"
                                      abseil_absl_meta_DEPS_TARGET
                                      abseil_absl_meta_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_meta")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::meta
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_meta_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_meta_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_meta_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::meta
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_meta_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::meta PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_meta_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::meta PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_meta_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::meta PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_meta_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::meta PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_meta_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::type_traits #############

        set(abseil_absl_type_traits_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_type_traits_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_type_traits_FRAMEWORKS_RELEASE}" "${abseil_absl_type_traits_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_type_traits_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_type_traits_DEPS_TARGET)
            add_library(abseil_absl_type_traits_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_type_traits_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_type_traits_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_type_traits_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_type_traits_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_type_traits_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_type_traits_LIBS_RELEASE}"
                                      "${abseil_absl_type_traits_LIB_DIRS_RELEASE}"
                                      abseil_absl_type_traits_DEPS_TARGET
                                      abseil_absl_type_traits_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_type_traits")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::type_traits
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_type_traits_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_type_traits_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_type_traits_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::type_traits
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_type_traits_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::type_traits PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_type_traits_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::type_traits PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_type_traits_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::type_traits PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_type_traits_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::type_traits PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_type_traits_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::flags_commandlineflag_internal #############

        set(abseil_absl_flags_commandlineflag_internal_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_flags_commandlineflag_internal_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_flags_commandlineflag_internal_FRAMEWORKS_RELEASE}" "${abseil_absl_flags_commandlineflag_internal_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_flags_commandlineflag_internal_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_flags_commandlineflag_internal_DEPS_TARGET)
            add_library(abseil_absl_flags_commandlineflag_internal_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_flags_commandlineflag_internal_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_flags_commandlineflag_internal_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_flags_commandlineflag_internal_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_flags_commandlineflag_internal_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_flags_commandlineflag_internal_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_flags_commandlineflag_internal_LIBS_RELEASE}"
                                      "${abseil_absl_flags_commandlineflag_internal_LIB_DIRS_RELEASE}"
                                      abseil_absl_flags_commandlineflag_internal_DEPS_TARGET
                                      abseil_absl_flags_commandlineflag_internal_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_flags_commandlineflag_internal")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::flags_commandlineflag_internal
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_flags_commandlineflag_internal_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_flags_commandlineflag_internal_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_flags_commandlineflag_internal_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::flags_commandlineflag_internal
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_flags_commandlineflag_internal_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::flags_commandlineflag_internal PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_flags_commandlineflag_internal_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::flags_commandlineflag_internal PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_flags_commandlineflag_internal_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::flags_commandlineflag_internal PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_flags_commandlineflag_internal_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::flags_commandlineflag_internal PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_flags_commandlineflag_internal_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::leak_check #############

        set(abseil_absl_leak_check_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_leak_check_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_leak_check_FRAMEWORKS_RELEASE}" "${abseil_absl_leak_check_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_leak_check_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_leak_check_DEPS_TARGET)
            add_library(abseil_absl_leak_check_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_leak_check_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_leak_check_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_leak_check_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_leak_check_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_leak_check_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_leak_check_LIBS_RELEASE}"
                                      "${abseil_absl_leak_check_LIB_DIRS_RELEASE}"
                                      abseil_absl_leak_check_DEPS_TARGET
                                      abseil_absl_leak_check_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_leak_check")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::leak_check
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_leak_check_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_leak_check_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_leak_check_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::leak_check
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_leak_check_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::leak_check PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_leak_check_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::leak_check PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_leak_check_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::leak_check PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_leak_check_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::leak_check PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_leak_check_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::node_slot_policy #############

        set(abseil_absl_node_slot_policy_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_node_slot_policy_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_node_slot_policy_FRAMEWORKS_RELEASE}" "${abseil_absl_node_slot_policy_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_node_slot_policy_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_node_slot_policy_DEPS_TARGET)
            add_library(abseil_absl_node_slot_policy_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_node_slot_policy_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_node_slot_policy_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_node_slot_policy_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_node_slot_policy_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_node_slot_policy_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_node_slot_policy_LIBS_RELEASE}"
                                      "${abseil_absl_node_slot_policy_LIB_DIRS_RELEASE}"
                                      abseil_absl_node_slot_policy_DEPS_TARGET
                                      abseil_absl_node_slot_policy_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_node_slot_policy")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::node_slot_policy
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_node_slot_policy_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_node_slot_policy_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_node_slot_policy_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::node_slot_policy
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_node_slot_policy_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::node_slot_policy PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_node_slot_policy_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::node_slot_policy PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_node_slot_policy_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::node_slot_policy PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_node_slot_policy_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::node_slot_policy PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_node_slot_policy_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::hashtable_debug_hooks #############

        set(abseil_absl_hashtable_debug_hooks_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_hashtable_debug_hooks_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_hashtable_debug_hooks_FRAMEWORKS_RELEASE}" "${abseil_absl_hashtable_debug_hooks_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_hashtable_debug_hooks_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_hashtable_debug_hooks_DEPS_TARGET)
            add_library(abseil_absl_hashtable_debug_hooks_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_hashtable_debug_hooks_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_hashtable_debug_hooks_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_hashtable_debug_hooks_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_hashtable_debug_hooks_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_hashtable_debug_hooks_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_hashtable_debug_hooks_LIBS_RELEASE}"
                                      "${abseil_absl_hashtable_debug_hooks_LIB_DIRS_RELEASE}"
                                      abseil_absl_hashtable_debug_hooks_DEPS_TARGET
                                      abseil_absl_hashtable_debug_hooks_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_hashtable_debug_hooks")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::hashtable_debug_hooks
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_hashtable_debug_hooks_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_hashtable_debug_hooks_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_hashtable_debug_hooks_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::hashtable_debug_hooks
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_hashtable_debug_hooks_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::hashtable_debug_hooks PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_hashtable_debug_hooks_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::hashtable_debug_hooks PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_hashtable_debug_hooks_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::hashtable_debug_hooks PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_hashtable_debug_hooks_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::hashtable_debug_hooks PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_hashtable_debug_hooks_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::counting_allocator #############

        set(abseil_absl_counting_allocator_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_counting_allocator_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_counting_allocator_FRAMEWORKS_RELEASE}" "${abseil_absl_counting_allocator_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_counting_allocator_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_counting_allocator_DEPS_TARGET)
            add_library(abseil_absl_counting_allocator_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_counting_allocator_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_counting_allocator_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_counting_allocator_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_counting_allocator_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_counting_allocator_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_counting_allocator_LIBS_RELEASE}"
                                      "${abseil_absl_counting_allocator_LIB_DIRS_RELEASE}"
                                      abseil_absl_counting_allocator_DEPS_TARGET
                                      abseil_absl_counting_allocator_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_counting_allocator")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::counting_allocator
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_counting_allocator_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_counting_allocator_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_counting_allocator_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::counting_allocator
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_counting_allocator_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::counting_allocator PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_counting_allocator_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::counting_allocator PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_counting_allocator_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::counting_allocator PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_counting_allocator_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::counting_allocator PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_counting_allocator_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::algorithm #############

        set(abseil_absl_algorithm_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_algorithm_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_algorithm_FRAMEWORKS_RELEASE}" "${abseil_absl_algorithm_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_algorithm_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_algorithm_DEPS_TARGET)
            add_library(abseil_absl_algorithm_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_algorithm_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_algorithm_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_algorithm_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_algorithm_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_algorithm_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_algorithm_LIBS_RELEASE}"
                                      "${abseil_absl_algorithm_LIB_DIRS_RELEASE}"
                                      abseil_absl_algorithm_DEPS_TARGET
                                      abseil_absl_algorithm_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_algorithm")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::algorithm
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_algorithm_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_algorithm_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_algorithm_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::algorithm
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_algorithm_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::algorithm PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_algorithm_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::algorithm PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_algorithm_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::algorithm PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_algorithm_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::algorithm PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_algorithm_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::prefetch #############

        set(abseil_absl_prefetch_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_prefetch_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_prefetch_FRAMEWORKS_RELEASE}" "${abseil_absl_prefetch_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_prefetch_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_prefetch_DEPS_TARGET)
            add_library(abseil_absl_prefetch_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_prefetch_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_prefetch_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_prefetch_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_prefetch_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_prefetch_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_prefetch_LIBS_RELEASE}"
                                      "${abseil_absl_prefetch_LIB_DIRS_RELEASE}"
                                      abseil_absl_prefetch_DEPS_TARGET
                                      abseil_absl_prefetch_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_prefetch")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::prefetch
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_prefetch_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_prefetch_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_prefetch_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::prefetch
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_prefetch_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::prefetch PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_prefetch_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::prefetch PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_prefetch_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::prefetch PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_prefetch_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::prefetch PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_prefetch_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::fast_type_id #############

        set(abseil_absl_fast_type_id_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_fast_type_id_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_fast_type_id_FRAMEWORKS_RELEASE}" "${abseil_absl_fast_type_id_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_fast_type_id_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_fast_type_id_DEPS_TARGET)
            add_library(abseil_absl_fast_type_id_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_fast_type_id_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_fast_type_id_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_fast_type_id_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_fast_type_id_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_fast_type_id_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_fast_type_id_LIBS_RELEASE}"
                                      "${abseil_absl_fast_type_id_LIB_DIRS_RELEASE}"
                                      abseil_absl_fast_type_id_DEPS_TARGET
                                      abseil_absl_fast_type_id_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_fast_type_id")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::fast_type_id
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_fast_type_id_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_fast_type_id_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_fast_type_id_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::fast_type_id
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_fast_type_id_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::fast_type_id PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_fast_type_id_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::fast_type_id PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_fast_type_id_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::fast_type_id PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_fast_type_id_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::fast_type_id PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_fast_type_id_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::pretty_function #############

        set(abseil_absl_pretty_function_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_pretty_function_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_pretty_function_FRAMEWORKS_RELEASE}" "${abseil_absl_pretty_function_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_pretty_function_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_pretty_function_DEPS_TARGET)
            add_library(abseil_absl_pretty_function_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_pretty_function_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_pretty_function_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_pretty_function_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_pretty_function_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_pretty_function_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_pretty_function_LIBS_RELEASE}"
                                      "${abseil_absl_pretty_function_LIB_DIRS_RELEASE}"
                                      abseil_absl_pretty_function_DEPS_TARGET
                                      abseil_absl_pretty_function_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_pretty_function")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::pretty_function
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_pretty_function_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_pretty_function_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_pretty_function_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::pretty_function
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_pretty_function_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::pretty_function PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_pretty_function_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::pretty_function PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_pretty_function_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::pretty_function PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_pretty_function_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::pretty_function PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_pretty_function_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::core_headers #############

        set(abseil_absl_core_headers_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_core_headers_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_core_headers_FRAMEWORKS_RELEASE}" "${abseil_absl_core_headers_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_core_headers_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_core_headers_DEPS_TARGET)
            add_library(abseil_absl_core_headers_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_core_headers_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_core_headers_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_core_headers_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_core_headers_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_core_headers_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_core_headers_LIBS_RELEASE}"
                                      "${abseil_absl_core_headers_LIB_DIRS_RELEASE}"
                                      abseil_absl_core_headers_DEPS_TARGET
                                      abseil_absl_core_headers_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_core_headers")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::core_headers
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_core_headers_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_core_headers_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_core_headers_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::core_headers
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_core_headers_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::core_headers PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_core_headers_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::core_headers PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_core_headers_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::core_headers PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_core_headers_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::core_headers PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_core_headers_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::dynamic_annotations #############

        set(abseil_absl_dynamic_annotations_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_dynamic_annotations_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_dynamic_annotations_FRAMEWORKS_RELEASE}" "${abseil_absl_dynamic_annotations_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_dynamic_annotations_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_dynamic_annotations_DEPS_TARGET)
            add_library(abseil_absl_dynamic_annotations_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_dynamic_annotations_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_dynamic_annotations_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_dynamic_annotations_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_dynamic_annotations_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_dynamic_annotations_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_dynamic_annotations_LIBS_RELEASE}"
                                      "${abseil_absl_dynamic_annotations_LIB_DIRS_RELEASE}"
                                      abseil_absl_dynamic_annotations_DEPS_TARGET
                                      abseil_absl_dynamic_annotations_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_dynamic_annotations")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::dynamic_annotations
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_dynamic_annotations_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_dynamic_annotations_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_dynamic_annotations_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::dynamic_annotations
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_dynamic_annotations_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::dynamic_annotations PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_dynamic_annotations_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::dynamic_annotations PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_dynamic_annotations_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::dynamic_annotations PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_dynamic_annotations_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::dynamic_annotations PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_dynamic_annotations_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT absl::config #############

        set(abseil_absl_config_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(abseil_absl_config_FRAMEWORKS_FOUND_RELEASE "${abseil_absl_config_FRAMEWORKS_RELEASE}" "${abseil_absl_config_FRAMEWORK_DIRS_RELEASE}")

        set(abseil_absl_config_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET abseil_absl_config_DEPS_TARGET)
            add_library(abseil_absl_config_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET abseil_absl_config_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_config_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_config_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_config_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'abseil_absl_config_DEPS_TARGET' to all of them
        conan_package_library_targets("${abseil_absl_config_LIBS_RELEASE}"
                                      "${abseil_absl_config_LIB_DIRS_RELEASE}"
                                      abseil_absl_config_DEPS_TARGET
                                      abseil_absl_config_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "abseil_absl_config")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET absl::config
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${abseil_absl_config_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${abseil_absl_config_LIBRARIES_TARGETS}>
                     APPEND)

        if("${abseil_absl_config_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET absl::config
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         abseil_absl_config_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET absl::config PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_config_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET absl::config PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${abseil_absl_config_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET absl::config PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${abseil_absl_config_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET absl::config PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${abseil_absl_config_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## AGGREGATED GLOBAL TARGET WITH THE COMPONENTS #####################
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::flags_parse APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::flags_usage APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::flags_usage_internal APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::flags APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::flags_reflection APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::node_hash_map APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::flat_hash_map APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::raw_hash_map APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::node_hash_set APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::flat_hash_set APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::statusor APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::status APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::random_random APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::raw_hash_set APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::hashtablez_sampler APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::hash_function_defaults APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::btree APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::cord APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::cordz_update_scope APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::cordz_sample_token APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::cordz_info APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::cordz_handle APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::cordz_statistics APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::random_internal_distribution_test_util APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::random_seed_sequences APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::sample_recorder APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::flags_internal APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::flags_marshalling APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::flags_config APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::flags_program_name APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::synchronization APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::kernel_timeout_internal APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::cord_internal APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::str_format APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::random_internal_nonsecure_base APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::random_internal_salted_seed_seq APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::random_internal_pool_urbg APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::random_internal_seed_material APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::random_distributions APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::hash APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::flags_private_handle_accessor APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::flags_commandlineflag APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::flags_path_util APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::failure_signal_handler APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::examine_stack APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::symbolize APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::layout APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::any APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::time APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::graphcycles_internal APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::str_format_internal APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::strings APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::random_internal_randen_engine APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::random_internal_mock_helpers APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::random_bit_gen_ref APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::malloc_internal APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::variant APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::optional APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::bad_any_cast APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::strings_internal APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::random_internal_randen APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::random_internal_distribution_caller APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::low_level_hash APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::city APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::bind_front APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::any_invocable APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::debugging APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::demangle_internal APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::stacktrace APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::container_memory APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::inlined_vector APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::inlined_vector_internal APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::fixed_array APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::compressed_tuple APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::cleanup APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::cleanup_internal APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::endian APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::base APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::spinlock_wait APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::utility APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::bad_variant_access APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::bad_optional_access APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::span APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::bad_any_cast_impl APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::cordz_functions APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::random_internal_randen_hwaes APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::random_internal_generate_real APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::memory APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::function_ref APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::debugging_internal APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::container_common APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::hashtable_debug APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::hash_policy_traits APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::algorithm_container APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::strerror APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::scoped_set_env APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::throw_delegate APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::base_internal APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::raw_logging_internal APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::log_severity APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::errno_saver APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::atomic_hook APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::compare APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::time_zone APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::civil_time APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::cordz_update_tracker APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::random_internal_uniform_helper APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::random_internal_randen_hwaes_impl APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::random_internal_randen_slow APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::random_internal_platform APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::random_internal_pcg_engine APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::random_internal_fastmath APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::random_internal_wide_multiply APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::random_internal_iostream_state_saver APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::random_internal_fast_uniform_bits APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::random_internal_traits APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::random_seed_gen_exception APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::periodic_sampler APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::exponential_biased APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::numeric_representation APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::numeric APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::int128 APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::bits APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::meta APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::type_traits APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::flags_commandlineflag_internal APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::leak_check APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::node_slot_policy APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::hashtable_debug_hooks APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::counting_allocator APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::algorithm APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::prefetch APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::fast_type_id APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::pretty_function APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::core_headers APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::dynamic_annotations APPEND)
    set_property(TARGET abseil::abseil PROPERTY INTERFACE_LINK_LIBRARIES absl::config APPEND)

########## For the modules (FindXXX)
set(abseil_LIBRARIES_RELEASE abseil::abseil)
