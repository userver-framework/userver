# Avoid multiple calls to find_package to append duplicated properties to the targets
include_guard()########### VARIABLES #######################################################################
#############################################################################################
set(cyrus-sasl_FRAMEWORKS_FOUND_RELEASE "") # Will be filled later
conan_find_apple_frameworks(cyrus-sasl_FRAMEWORKS_FOUND_RELEASE "${cyrus-sasl_FRAMEWORKS_RELEASE}" "${cyrus-sasl_FRAMEWORK_DIRS_RELEASE}")

set(cyrus-sasl_LIBRARIES_TARGETS "") # Will be filled later


######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
if(NOT TARGET cyrus-sasl_DEPS_TARGET)
    add_library(cyrus-sasl_DEPS_TARGET INTERFACE IMPORTED)
endif()

set_property(TARGET cyrus-sasl_DEPS_TARGET
             PROPERTY INTERFACE_LINK_LIBRARIES
             $<$<CONFIG:Release>:${cyrus-sasl_FRAMEWORKS_FOUND_RELEASE}>
             $<$<CONFIG:Release>:${cyrus-sasl_SYSTEM_LIBS_RELEASE}>
             $<$<CONFIG:Release>:openssl::openssl>
             APPEND)

####### Find the libraries declared in cpp_info.libs, create an IMPORTED target for each one and link the
####### cyrus-sasl_DEPS_TARGET to all of them
conan_package_library_targets("${cyrus-sasl_LIBS_RELEASE}"    # libraries
                              "${cyrus-sasl_LIB_DIRS_RELEASE}" # package_libdir
                              cyrus-sasl_DEPS_TARGET
                              cyrus-sasl_LIBRARIES_TARGETS  # out_libraries_targets
                              "_RELEASE"
                              "cyrus-sasl")    # package_name

# FIXME: What is the result of this for multi-config? All configs adding themselves to path?
set(CMAKE_MODULE_PATH ${cyrus-sasl_BUILD_DIRS_RELEASE} ${CMAKE_MODULE_PATH})

########## GLOBAL TARGET PROPERTIES Release ########################################
    set_property(TARGET cyrus-sasl::cyrus-sasl
                 PROPERTY INTERFACE_LINK_LIBRARIES
                 $<$<CONFIG:Release>:${cyrus-sasl_OBJECTS_RELEASE}>
                 $<$<CONFIG:Release>:${cyrus-sasl_LIBRARIES_TARGETS}>
                 APPEND)

    if("${cyrus-sasl_LIBS_RELEASE}" STREQUAL "")
        # If the package is not declaring any "cpp_info.libs" the package deps, system libs,
        # frameworks etc are not linked to the imported targets and we need to do it to the
        # global target
        set_property(TARGET cyrus-sasl::cyrus-sasl
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     cyrus-sasl_DEPS_TARGET
                     APPEND)
    endif()

    set_property(TARGET cyrus-sasl::cyrus-sasl
                 PROPERTY INTERFACE_LINK_OPTIONS
                 $<$<CONFIG:Release>:${cyrus-sasl_LINKER_FLAGS_RELEASE}> APPEND)
    set_property(TARGET cyrus-sasl::cyrus-sasl
                 PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                 $<$<CONFIG:Release>:${cyrus-sasl_INCLUDE_DIRS_RELEASE}> APPEND)
    set_property(TARGET cyrus-sasl::cyrus-sasl
                 PROPERTY INTERFACE_COMPILE_DEFINITIONS
                 $<$<CONFIG:Release>:${cyrus-sasl_COMPILE_DEFINITIONS_RELEASE}> APPEND)
    set_property(TARGET cyrus-sasl::cyrus-sasl
                 PROPERTY INTERFACE_COMPILE_OPTIONS
                 $<$<CONFIG:Release>:${cyrus-sasl_COMPILE_OPTIONS_RELEASE}> APPEND)

########## For the modules (FindXXX)
set(cyrus-sasl_LIBRARIES_RELEASE cyrus-sasl::cyrus-sasl)
