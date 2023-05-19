# Avoid multiple calls to find_package to append duplicated properties to the targets
include_guard()########### VARIABLES #######################################################################
#############################################################################################
set(grpc-proto_FRAMEWORKS_FOUND_RELEASE "") # Will be filled later
conan_find_apple_frameworks(grpc-proto_FRAMEWORKS_FOUND_RELEASE "${grpc-proto_FRAMEWORKS_RELEASE}" "${grpc-proto_FRAMEWORK_DIRS_RELEASE}")

set(grpc-proto_LIBRARIES_TARGETS "") # Will be filled later


######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
if(NOT TARGET grpc-proto_DEPS_TARGET)
    add_library(grpc-proto_DEPS_TARGET INTERFACE IMPORTED)
endif()

set_property(TARGET grpc-proto_DEPS_TARGET
             PROPERTY INTERFACE_LINK_LIBRARIES
             $<$<CONFIG:Release>:${grpc-proto_FRAMEWORKS_FOUND_RELEASE}>
             $<$<CONFIG:Release>:${grpc-proto_SYSTEM_LIBS_RELEASE}>
             $<$<CONFIG:Release>:protobuf::protobuf;googleapis::googleapis>
             APPEND)

####### Find the libraries declared in cpp_info.libs, create an IMPORTED target for each one and link the
####### grpc-proto_DEPS_TARGET to all of them
conan_package_library_targets("${grpc-proto_LIBS_RELEASE}"    # libraries
                              "${grpc-proto_LIB_DIRS_RELEASE}" # package_libdir
                              grpc-proto_DEPS_TARGET
                              grpc-proto_LIBRARIES_TARGETS  # out_libraries_targets
                              "_RELEASE"
                              "grpc-proto")    # package_name

# FIXME: What is the result of this for multi-config? All configs adding themselves to path?
set(CMAKE_MODULE_PATH ${grpc-proto_BUILD_DIRS_RELEASE} ${CMAKE_MODULE_PATH})

########## GLOBAL TARGET PROPERTIES Release ########################################
    set_property(TARGET grpc-proto::grpc-proto
                 PROPERTY INTERFACE_LINK_LIBRARIES
                 $<$<CONFIG:Release>:${grpc-proto_OBJECTS_RELEASE}>
                 $<$<CONFIG:Release>:${grpc-proto_LIBRARIES_TARGETS}>
                 APPEND)

    if("${grpc-proto_LIBS_RELEASE}" STREQUAL "")
        # If the package is not declaring any "cpp_info.libs" the package deps, system libs,
        # frameworks etc are not linked to the imported targets and we need to do it to the
        # global target
        set_property(TARGET grpc-proto::grpc-proto
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     grpc-proto_DEPS_TARGET
                     APPEND)
    endif()

    set_property(TARGET grpc-proto::grpc-proto
                 PROPERTY INTERFACE_LINK_OPTIONS
                 $<$<CONFIG:Release>:${grpc-proto_LINKER_FLAGS_RELEASE}> APPEND)
    set_property(TARGET grpc-proto::grpc-proto
                 PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                 $<$<CONFIG:Release>:${grpc-proto_INCLUDE_DIRS_RELEASE}> APPEND)
    set_property(TARGET grpc-proto::grpc-proto
                 PROPERTY INTERFACE_COMPILE_DEFINITIONS
                 $<$<CONFIG:Release>:${grpc-proto_COMPILE_DEFINITIONS_RELEASE}> APPEND)
    set_property(TARGET grpc-proto::grpc-proto
                 PROPERTY INTERFACE_COMPILE_OPTIONS
                 $<$<CONFIG:Release>:${grpc-proto_COMPILE_OPTIONS_RELEASE}> APPEND)

########## For the modules (FindXXX)
set(grpc-proto_LIBRARIES_RELEASE grpc-proto::grpc-proto)
