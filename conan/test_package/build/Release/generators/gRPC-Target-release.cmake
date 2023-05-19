# Avoid multiple calls to find_package to append duplicated properties to the targets
include_guard()########### VARIABLES #######################################################################
#############################################################################################
set(grpc_FRAMEWORKS_FOUND_RELEASE "") # Will be filled later
conan_find_apple_frameworks(grpc_FRAMEWORKS_FOUND_RELEASE "${grpc_FRAMEWORKS_RELEASE}" "${grpc_FRAMEWORK_DIRS_RELEASE}")

set(grpc_LIBRARIES_TARGETS "") # Will be filled later


######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
if(NOT TARGET grpc_DEPS_TARGET)
    add_library(grpc_DEPS_TARGET INTERFACE IMPORTED)
endif()

set_property(TARGET grpc_DEPS_TARGET
             PROPERTY INTERFACE_LINK_LIBRARIES
             $<$<CONFIG:Release>:${grpc_FRAMEWORKS_FOUND_RELEASE}>
             $<$<CONFIG:Release>:${grpc_SYSTEM_LIBS_RELEASE}>
             $<$<CONFIG:Release>:absl::base;absl::memory;absl::status;absl::str_format;absl::strings;absl::synchronization;absl::time;absl::optional;gRPC::upb;absl::bind_front;absl::flat_hash_map;absl::inlined_vector;absl::statusor;absl::random_random;c-ares::cares;OpenSSL::Crypto;OpenSSL::SSL;re2::re2;ZLIB::ZLIB;gRPC::address_sorting;gRPC::gpr;gRPC::upb;protobuf::libprotobuf;gRPC::grpc;protobuf::libprotobuf;gRPC::grpc++;protobuf::libprotobuf;gRPC::grpc++;protobuf::libprotoc;protobuf::libprotobuf;absl::flat_hash_map;absl::inlined_vector;absl::statusor;c-ares::cares;re2::re2;ZLIB::ZLIB;absl::random_random;gRPC::address_sorting;gRPC::gpr;gRPC::upb;protobuf::libprotobuf;gRPC::grpc_unsecure;protobuf::libprotobuf;grpc-proto::grpc-proto;googleapis::googleapis;gRPC::grpc++;protobuf::libprotobuf;grpc-proto::grpc-proto;googleapis::googleapis;gRPC::grpc++>
             APPEND)

####### Find the libraries declared in cpp_info.libs, create an IMPORTED target for each one and link the
####### grpc_DEPS_TARGET to all of them
conan_package_library_targets("${grpc_LIBS_RELEASE}"    # libraries
                              "${grpc_LIB_DIRS_RELEASE}" # package_libdir
                              grpc_DEPS_TARGET
                              grpc_LIBRARIES_TARGETS  # out_libraries_targets
                              "_RELEASE"
                              "grpc")    # package_name

# FIXME: What is the result of this for multi-config? All configs adding themselves to path?
set(CMAKE_MODULE_PATH ${grpc_BUILD_DIRS_RELEASE} ${CMAKE_MODULE_PATH})

########## COMPONENTS TARGET PROPERTIES Release ########################################

    ########## COMPONENT gRPC::grpcpp_channelz #############

        set(grpc_gRPC_grpcpp_channelz_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(grpc_gRPC_grpcpp_channelz_FRAMEWORKS_FOUND_RELEASE "${grpc_gRPC_grpcpp_channelz_FRAMEWORKS_RELEASE}" "${grpc_gRPC_grpcpp_channelz_FRAMEWORK_DIRS_RELEASE}")

        set(grpc_gRPC_grpcpp_channelz_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET grpc_gRPC_grpcpp_channelz_DEPS_TARGET)
            add_library(grpc_gRPC_grpcpp_channelz_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET grpc_gRPC_grpcpp_channelz_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${grpc_gRPC_grpcpp_channelz_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${grpc_gRPC_grpcpp_channelz_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${grpc_gRPC_grpcpp_channelz_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'grpc_gRPC_grpcpp_channelz_DEPS_TARGET' to all of them
        conan_package_library_targets("${grpc_gRPC_grpcpp_channelz_LIBS_RELEASE}"
                                      "${grpc_gRPC_grpcpp_channelz_LIB_DIRS_RELEASE}"
                                      grpc_gRPC_grpcpp_channelz_DEPS_TARGET
                                      grpc_gRPC_grpcpp_channelz_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "grpc_gRPC_grpcpp_channelz")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET gRPC::grpcpp_channelz
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${grpc_gRPC_grpcpp_channelz_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${grpc_gRPC_grpcpp_channelz_LIBRARIES_TARGETS}>
                     APPEND)

        if("${grpc_gRPC_grpcpp_channelz_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET gRPC::grpcpp_channelz
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         grpc_gRPC_grpcpp_channelz_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET gRPC::grpcpp_channelz PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${grpc_gRPC_grpcpp_channelz_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET gRPC::grpcpp_channelz PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${grpc_gRPC_grpcpp_channelz_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET gRPC::grpcpp_channelz PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${grpc_gRPC_grpcpp_channelz_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET gRPC::grpcpp_channelz PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${grpc_gRPC_grpcpp_channelz_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT gRPC::grpc++_reflection #############

        set(grpc_gRPC_grpc++_reflection_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(grpc_gRPC_grpc++_reflection_FRAMEWORKS_FOUND_RELEASE "${grpc_gRPC_grpc++_reflection_FRAMEWORKS_RELEASE}" "${grpc_gRPC_grpc++_reflection_FRAMEWORK_DIRS_RELEASE}")

        set(grpc_gRPC_grpc++_reflection_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET grpc_gRPC_grpc++_reflection_DEPS_TARGET)
            add_library(grpc_gRPC_grpc++_reflection_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET grpc_gRPC_grpc++_reflection_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc++_reflection_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc++_reflection_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc++_reflection_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'grpc_gRPC_grpc++_reflection_DEPS_TARGET' to all of them
        conan_package_library_targets("${grpc_gRPC_grpc++_reflection_LIBS_RELEASE}"
                                      "${grpc_gRPC_grpc++_reflection_LIB_DIRS_RELEASE}"
                                      grpc_gRPC_grpc++_reflection_DEPS_TARGET
                                      grpc_gRPC_grpc++_reflection_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "grpc_gRPC_grpc++_reflection")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET gRPC::grpc++_reflection
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc++_reflection_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc++_reflection_LIBRARIES_TARGETS}>
                     APPEND)

        if("${grpc_gRPC_grpc++_reflection_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET gRPC::grpc++_reflection
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         grpc_gRPC_grpc++_reflection_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET gRPC::grpc++_reflection PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc++_reflection_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET gRPC::grpc++_reflection PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc++_reflection_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET gRPC::grpc++_reflection PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc++_reflection_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET gRPC::grpc++_reflection PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc++_reflection_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT gRPC::grpc++_unsecure #############

        set(grpc_gRPC_grpc++_unsecure_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(grpc_gRPC_grpc++_unsecure_FRAMEWORKS_FOUND_RELEASE "${grpc_gRPC_grpc++_unsecure_FRAMEWORKS_RELEASE}" "${grpc_gRPC_grpc++_unsecure_FRAMEWORK_DIRS_RELEASE}")

        set(grpc_gRPC_grpc++_unsecure_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET grpc_gRPC_grpc++_unsecure_DEPS_TARGET)
            add_library(grpc_gRPC_grpc++_unsecure_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET grpc_gRPC_grpc++_unsecure_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc++_unsecure_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc++_unsecure_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc++_unsecure_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'grpc_gRPC_grpc++_unsecure_DEPS_TARGET' to all of them
        conan_package_library_targets("${grpc_gRPC_grpc++_unsecure_LIBS_RELEASE}"
                                      "${grpc_gRPC_grpc++_unsecure_LIB_DIRS_RELEASE}"
                                      grpc_gRPC_grpc++_unsecure_DEPS_TARGET
                                      grpc_gRPC_grpc++_unsecure_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "grpc_gRPC_grpc++_unsecure")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET gRPC::grpc++_unsecure
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc++_unsecure_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc++_unsecure_LIBRARIES_TARGETS}>
                     APPEND)

        if("${grpc_gRPC_grpc++_unsecure_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET gRPC::grpc++_unsecure
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         grpc_gRPC_grpc++_unsecure_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET gRPC::grpc++_unsecure PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc++_unsecure_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET gRPC::grpc++_unsecure PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc++_unsecure_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET gRPC::grpc++_unsecure PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc++_unsecure_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET gRPC::grpc++_unsecure PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc++_unsecure_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT gRPC::grpc_unsecure #############

        set(grpc_gRPC_grpc_unsecure_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(grpc_gRPC_grpc_unsecure_FRAMEWORKS_FOUND_RELEASE "${grpc_gRPC_grpc_unsecure_FRAMEWORKS_RELEASE}" "${grpc_gRPC_grpc_unsecure_FRAMEWORK_DIRS_RELEASE}")

        set(grpc_gRPC_grpc_unsecure_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET grpc_gRPC_grpc_unsecure_DEPS_TARGET)
            add_library(grpc_gRPC_grpc_unsecure_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET grpc_gRPC_grpc_unsecure_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc_unsecure_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc_unsecure_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc_unsecure_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'grpc_gRPC_grpc_unsecure_DEPS_TARGET' to all of them
        conan_package_library_targets("${grpc_gRPC_grpc_unsecure_LIBS_RELEASE}"
                                      "${grpc_gRPC_grpc_unsecure_LIB_DIRS_RELEASE}"
                                      grpc_gRPC_grpc_unsecure_DEPS_TARGET
                                      grpc_gRPC_grpc_unsecure_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "grpc_gRPC_grpc_unsecure")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET gRPC::grpc_unsecure
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc_unsecure_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc_unsecure_LIBRARIES_TARGETS}>
                     APPEND)

        if("${grpc_gRPC_grpc_unsecure_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET gRPC::grpc_unsecure
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         grpc_gRPC_grpc_unsecure_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET gRPC::grpc_unsecure PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc_unsecure_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET gRPC::grpc_unsecure PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc_unsecure_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET gRPC::grpc_unsecure PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc_unsecure_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET gRPC::grpc_unsecure PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc_unsecure_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT gRPC::grpc++_error_details #############

        set(grpc_gRPC_grpc++_error_details_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(grpc_gRPC_grpc++_error_details_FRAMEWORKS_FOUND_RELEASE "${grpc_gRPC_grpc++_error_details_FRAMEWORKS_RELEASE}" "${grpc_gRPC_grpc++_error_details_FRAMEWORK_DIRS_RELEASE}")

        set(grpc_gRPC_grpc++_error_details_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET grpc_gRPC_grpc++_error_details_DEPS_TARGET)
            add_library(grpc_gRPC_grpc++_error_details_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET grpc_gRPC_grpc++_error_details_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc++_error_details_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc++_error_details_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc++_error_details_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'grpc_gRPC_grpc++_error_details_DEPS_TARGET' to all of them
        conan_package_library_targets("${grpc_gRPC_grpc++_error_details_LIBS_RELEASE}"
                                      "${grpc_gRPC_grpc++_error_details_LIB_DIRS_RELEASE}"
                                      grpc_gRPC_grpc++_error_details_DEPS_TARGET
                                      grpc_gRPC_grpc++_error_details_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "grpc_gRPC_grpc++_error_details")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET gRPC::grpc++_error_details
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc++_error_details_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc++_error_details_LIBRARIES_TARGETS}>
                     APPEND)

        if("${grpc_gRPC_grpc++_error_details_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET gRPC::grpc++_error_details
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         grpc_gRPC_grpc++_error_details_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET gRPC::grpc++_error_details PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc++_error_details_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET gRPC::grpc++_error_details PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc++_error_details_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET gRPC::grpc++_error_details PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc++_error_details_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET gRPC::grpc++_error_details PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc++_error_details_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT gRPC::grpc++_alts #############

        set(grpc_gRPC_grpc++_alts_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(grpc_gRPC_grpc++_alts_FRAMEWORKS_FOUND_RELEASE "${grpc_gRPC_grpc++_alts_FRAMEWORKS_RELEASE}" "${grpc_gRPC_grpc++_alts_FRAMEWORK_DIRS_RELEASE}")

        set(grpc_gRPC_grpc++_alts_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET grpc_gRPC_grpc++_alts_DEPS_TARGET)
            add_library(grpc_gRPC_grpc++_alts_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET grpc_gRPC_grpc++_alts_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc++_alts_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc++_alts_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc++_alts_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'grpc_gRPC_grpc++_alts_DEPS_TARGET' to all of them
        conan_package_library_targets("${grpc_gRPC_grpc++_alts_LIBS_RELEASE}"
                                      "${grpc_gRPC_grpc++_alts_LIB_DIRS_RELEASE}"
                                      grpc_gRPC_grpc++_alts_DEPS_TARGET
                                      grpc_gRPC_grpc++_alts_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "grpc_gRPC_grpc++_alts")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET gRPC::grpc++_alts
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc++_alts_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc++_alts_LIBRARIES_TARGETS}>
                     APPEND)

        if("${grpc_gRPC_grpc++_alts_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET gRPC::grpc++_alts
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         grpc_gRPC_grpc++_alts_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET gRPC::grpc++_alts PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc++_alts_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET gRPC::grpc++_alts PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc++_alts_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET gRPC::grpc++_alts PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc++_alts_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET gRPC::grpc++_alts PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc++_alts_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT gRPC::grpc++ #############

        set(grpc_gRPC_grpc++_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(grpc_gRPC_grpc++_FRAMEWORKS_FOUND_RELEASE "${grpc_gRPC_grpc++_FRAMEWORKS_RELEASE}" "${grpc_gRPC_grpc++_FRAMEWORK_DIRS_RELEASE}")

        set(grpc_gRPC_grpc++_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET grpc_gRPC_grpc++_DEPS_TARGET)
            add_library(grpc_gRPC_grpc++_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET grpc_gRPC_grpc++_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc++_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc++_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc++_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'grpc_gRPC_grpc++_DEPS_TARGET' to all of them
        conan_package_library_targets("${grpc_gRPC_grpc++_LIBS_RELEASE}"
                                      "${grpc_gRPC_grpc++_LIB_DIRS_RELEASE}"
                                      grpc_gRPC_grpc++_DEPS_TARGET
                                      grpc_gRPC_grpc++_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "grpc_gRPC_grpc++")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET gRPC::grpc++
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc++_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc++_LIBRARIES_TARGETS}>
                     APPEND)

        if("${grpc_gRPC_grpc++_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET gRPC::grpc++
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         grpc_gRPC_grpc++_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET gRPC::grpc++ PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc++_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET gRPC::grpc++ PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc++_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET gRPC::grpc++ PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc++_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET gRPC::grpc++ PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc++_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT gRPC::grpc #############

        set(grpc_gRPC_grpc_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(grpc_gRPC_grpc_FRAMEWORKS_FOUND_RELEASE "${grpc_gRPC_grpc_FRAMEWORKS_RELEASE}" "${grpc_gRPC_grpc_FRAMEWORK_DIRS_RELEASE}")

        set(grpc_gRPC_grpc_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET grpc_gRPC_grpc_DEPS_TARGET)
            add_library(grpc_gRPC_grpc_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET grpc_gRPC_grpc_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'grpc_gRPC_grpc_DEPS_TARGET' to all of them
        conan_package_library_targets("${grpc_gRPC_grpc_LIBS_RELEASE}"
                                      "${grpc_gRPC_grpc_LIB_DIRS_RELEASE}"
                                      grpc_gRPC_grpc_DEPS_TARGET
                                      grpc_gRPC_grpc_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "grpc_gRPC_grpc")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET gRPC::grpc
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc_LIBRARIES_TARGETS}>
                     APPEND)

        if("${grpc_gRPC_grpc_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET gRPC::grpc
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         grpc_gRPC_grpc_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET gRPC::grpc PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET gRPC::grpc PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET gRPC::grpc PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET gRPC::grpc PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT gRPC::gpr #############

        set(grpc_gRPC_gpr_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(grpc_gRPC_gpr_FRAMEWORKS_FOUND_RELEASE "${grpc_gRPC_gpr_FRAMEWORKS_RELEASE}" "${grpc_gRPC_gpr_FRAMEWORK_DIRS_RELEASE}")

        set(grpc_gRPC_gpr_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET grpc_gRPC_gpr_DEPS_TARGET)
            add_library(grpc_gRPC_gpr_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET grpc_gRPC_gpr_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${grpc_gRPC_gpr_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${grpc_gRPC_gpr_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${grpc_gRPC_gpr_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'grpc_gRPC_gpr_DEPS_TARGET' to all of them
        conan_package_library_targets("${grpc_gRPC_gpr_LIBS_RELEASE}"
                                      "${grpc_gRPC_gpr_LIB_DIRS_RELEASE}"
                                      grpc_gRPC_gpr_DEPS_TARGET
                                      grpc_gRPC_gpr_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "grpc_gRPC_gpr")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET gRPC::gpr
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${grpc_gRPC_gpr_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${grpc_gRPC_gpr_LIBRARIES_TARGETS}>
                     APPEND)

        if("${grpc_gRPC_gpr_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET gRPC::gpr
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         grpc_gRPC_gpr_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET gRPC::gpr PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${grpc_gRPC_gpr_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET gRPC::gpr PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${grpc_gRPC_gpr_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET gRPC::gpr PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${grpc_gRPC_gpr_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET gRPC::gpr PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${grpc_gRPC_gpr_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT grpc::grpc_execs #############

        set(grpc_grpc_grpc_execs_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(grpc_grpc_grpc_execs_FRAMEWORKS_FOUND_RELEASE "${grpc_grpc_grpc_execs_FRAMEWORKS_RELEASE}" "${grpc_grpc_grpc_execs_FRAMEWORK_DIRS_RELEASE}")

        set(grpc_grpc_grpc_execs_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET grpc_grpc_grpc_execs_DEPS_TARGET)
            add_library(grpc_grpc_grpc_execs_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET grpc_grpc_grpc_execs_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${grpc_grpc_grpc_execs_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${grpc_grpc_grpc_execs_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${grpc_grpc_grpc_execs_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'grpc_grpc_grpc_execs_DEPS_TARGET' to all of them
        conan_package_library_targets("${grpc_grpc_grpc_execs_LIBS_RELEASE}"
                                      "${grpc_grpc_grpc_execs_LIB_DIRS_RELEASE}"
                                      grpc_grpc_grpc_execs_DEPS_TARGET
                                      grpc_grpc_grpc_execs_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "grpc_grpc_grpc_execs")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET grpc::grpc_execs
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${grpc_grpc_grpc_execs_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${grpc_grpc_grpc_execs_LIBRARIES_TARGETS}>
                     APPEND)

        if("${grpc_grpc_grpc_execs_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET grpc::grpc_execs
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         grpc_grpc_grpc_execs_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET grpc::grpc_execs PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${grpc_grpc_grpc_execs_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET grpc::grpc_execs PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${grpc_grpc_grpc_execs_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET grpc::grpc_execs PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${grpc_grpc_grpc_execs_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET grpc::grpc_execs PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${grpc_grpc_grpc_execs_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT gRPC::grpc_plugin_support #############

        set(grpc_gRPC_grpc_plugin_support_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(grpc_gRPC_grpc_plugin_support_FRAMEWORKS_FOUND_RELEASE "${grpc_gRPC_grpc_plugin_support_FRAMEWORKS_RELEASE}" "${grpc_gRPC_grpc_plugin_support_FRAMEWORK_DIRS_RELEASE}")

        set(grpc_gRPC_grpc_plugin_support_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET grpc_gRPC_grpc_plugin_support_DEPS_TARGET)
            add_library(grpc_gRPC_grpc_plugin_support_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET grpc_gRPC_grpc_plugin_support_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc_plugin_support_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc_plugin_support_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc_plugin_support_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'grpc_gRPC_grpc_plugin_support_DEPS_TARGET' to all of them
        conan_package_library_targets("${grpc_gRPC_grpc_plugin_support_LIBS_RELEASE}"
                                      "${grpc_gRPC_grpc_plugin_support_LIB_DIRS_RELEASE}"
                                      grpc_gRPC_grpc_plugin_support_DEPS_TARGET
                                      grpc_gRPC_grpc_plugin_support_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "grpc_gRPC_grpc_plugin_support")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET gRPC::grpc_plugin_support
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc_plugin_support_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc_plugin_support_LIBRARIES_TARGETS}>
                     APPEND)

        if("${grpc_gRPC_grpc_plugin_support_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET gRPC::grpc_plugin_support
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         grpc_gRPC_grpc_plugin_support_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET gRPC::grpc_plugin_support PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc_plugin_support_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET gRPC::grpc_plugin_support PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc_plugin_support_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET gRPC::grpc_plugin_support PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc_plugin_support_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET gRPC::grpc_plugin_support PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${grpc_gRPC_grpc_plugin_support_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT gRPC::upb #############

        set(grpc_gRPC_upb_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(grpc_gRPC_upb_FRAMEWORKS_FOUND_RELEASE "${grpc_gRPC_upb_FRAMEWORKS_RELEASE}" "${grpc_gRPC_upb_FRAMEWORK_DIRS_RELEASE}")

        set(grpc_gRPC_upb_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET grpc_gRPC_upb_DEPS_TARGET)
            add_library(grpc_gRPC_upb_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET grpc_gRPC_upb_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${grpc_gRPC_upb_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${grpc_gRPC_upb_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${grpc_gRPC_upb_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'grpc_gRPC_upb_DEPS_TARGET' to all of them
        conan_package_library_targets("${grpc_gRPC_upb_LIBS_RELEASE}"
                                      "${grpc_gRPC_upb_LIB_DIRS_RELEASE}"
                                      grpc_gRPC_upb_DEPS_TARGET
                                      grpc_gRPC_upb_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "grpc_gRPC_upb")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET gRPC::upb
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${grpc_gRPC_upb_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${grpc_gRPC_upb_LIBRARIES_TARGETS}>
                     APPEND)

        if("${grpc_gRPC_upb_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET gRPC::upb
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         grpc_gRPC_upb_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET gRPC::upb PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${grpc_gRPC_upb_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET gRPC::upb PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${grpc_gRPC_upb_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET gRPC::upb PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${grpc_gRPC_upb_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET gRPC::upb PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${grpc_gRPC_upb_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT gRPC::address_sorting #############

        set(grpc_gRPC_address_sorting_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(grpc_gRPC_address_sorting_FRAMEWORKS_FOUND_RELEASE "${grpc_gRPC_address_sorting_FRAMEWORKS_RELEASE}" "${grpc_gRPC_address_sorting_FRAMEWORK_DIRS_RELEASE}")

        set(grpc_gRPC_address_sorting_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET grpc_gRPC_address_sorting_DEPS_TARGET)
            add_library(grpc_gRPC_address_sorting_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET grpc_gRPC_address_sorting_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${grpc_gRPC_address_sorting_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${grpc_gRPC_address_sorting_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${grpc_gRPC_address_sorting_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'grpc_gRPC_address_sorting_DEPS_TARGET' to all of them
        conan_package_library_targets("${grpc_gRPC_address_sorting_LIBS_RELEASE}"
                                      "${grpc_gRPC_address_sorting_LIB_DIRS_RELEASE}"
                                      grpc_gRPC_address_sorting_DEPS_TARGET
                                      grpc_gRPC_address_sorting_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "grpc_gRPC_address_sorting")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET gRPC::address_sorting
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${grpc_gRPC_address_sorting_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${grpc_gRPC_address_sorting_LIBRARIES_TARGETS}>
                     APPEND)

        if("${grpc_gRPC_address_sorting_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET gRPC::address_sorting
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         grpc_gRPC_address_sorting_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET gRPC::address_sorting PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${grpc_gRPC_address_sorting_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET gRPC::address_sorting PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${grpc_gRPC_address_sorting_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET gRPC::address_sorting PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${grpc_gRPC_address_sorting_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET gRPC::address_sorting PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${grpc_gRPC_address_sorting_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## AGGREGATED GLOBAL TARGET WITH THE COMPONENTS #####################
    set_property(TARGET grpc::grpc PROPERTY INTERFACE_LINK_LIBRARIES gRPC::grpcpp_channelz APPEND)
    set_property(TARGET grpc::grpc PROPERTY INTERFACE_LINK_LIBRARIES gRPC::grpc++_reflection APPEND)
    set_property(TARGET grpc::grpc PROPERTY INTERFACE_LINK_LIBRARIES gRPC::grpc++_unsecure APPEND)
    set_property(TARGET grpc::grpc PROPERTY INTERFACE_LINK_LIBRARIES gRPC::grpc_unsecure APPEND)
    set_property(TARGET grpc::grpc PROPERTY INTERFACE_LINK_LIBRARIES gRPC::grpc++_error_details APPEND)
    set_property(TARGET grpc::grpc PROPERTY INTERFACE_LINK_LIBRARIES gRPC::grpc++_alts APPEND)
    set_property(TARGET grpc::grpc PROPERTY INTERFACE_LINK_LIBRARIES gRPC::grpc++ APPEND)
    set_property(TARGET grpc::grpc PROPERTY INTERFACE_LINK_LIBRARIES gRPC::grpc APPEND)
    set_property(TARGET grpc::grpc PROPERTY INTERFACE_LINK_LIBRARIES gRPC::gpr APPEND)
    set_property(TARGET grpc::grpc PROPERTY INTERFACE_LINK_LIBRARIES grpc::grpc_execs APPEND)
    set_property(TARGET grpc::grpc PROPERTY INTERFACE_LINK_LIBRARIES gRPC::grpc_plugin_support APPEND)
    set_property(TARGET grpc::grpc PROPERTY INTERFACE_LINK_LIBRARIES gRPC::upb APPEND)
    set_property(TARGET grpc::grpc PROPERTY INTERFACE_LINK_LIBRARIES gRPC::address_sorting APPEND)

########## For the modules (FindXXX)
set(grpc_LIBRARIES_RELEASE grpc::grpc)
