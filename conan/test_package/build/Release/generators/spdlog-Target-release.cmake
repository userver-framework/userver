# Avoid multiple calls to find_package to append duplicated properties to the targets
include_guard()########### VARIABLES #######################################################################
#############################################################################################
set(spdlog_FRAMEWORKS_FOUND_RELEASE "") # Will be filled later
conan_find_apple_frameworks(spdlog_FRAMEWORKS_FOUND_RELEASE "${spdlog_FRAMEWORKS_RELEASE}" "${spdlog_FRAMEWORK_DIRS_RELEASE}")

set(spdlog_LIBRARIES_TARGETS "") # Will be filled later


######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
if(NOT TARGET spdlog_DEPS_TARGET)
    add_library(spdlog_DEPS_TARGET INTERFACE IMPORTED)
endif()

set_property(TARGET spdlog_DEPS_TARGET
             PROPERTY INTERFACE_LINK_LIBRARIES
             $<$<CONFIG:Release>:${spdlog_FRAMEWORKS_FOUND_RELEASE}>
             $<$<CONFIG:Release>:${spdlog_SYSTEM_LIBS_RELEASE}>
             $<$<CONFIG:Release>:fmt::fmt>
             APPEND)

####### Find the libraries declared in cpp_info.libs, create an IMPORTED target for each one and link the
####### spdlog_DEPS_TARGET to all of them
conan_package_library_targets("${spdlog_LIBS_RELEASE}"    # libraries
                              "${spdlog_LIB_DIRS_RELEASE}" # package_libdir
                              spdlog_DEPS_TARGET
                              spdlog_LIBRARIES_TARGETS  # out_libraries_targets
                              "_RELEASE"
                              "spdlog")    # package_name

# FIXME: What is the result of this for multi-config? All configs adding themselves to path?
set(CMAKE_MODULE_PATH ${spdlog_BUILD_DIRS_RELEASE} ${CMAKE_MODULE_PATH})

########## COMPONENTS TARGET PROPERTIES Release ########################################

    ########## COMPONENT spdlog::libspdlog #############

        set(spdlog_spdlog_libspdlog_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(spdlog_spdlog_libspdlog_FRAMEWORKS_FOUND_RELEASE "${spdlog_spdlog_libspdlog_FRAMEWORKS_RELEASE}" "${spdlog_spdlog_libspdlog_FRAMEWORK_DIRS_RELEASE}")

        set(spdlog_spdlog_libspdlog_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET spdlog_spdlog_libspdlog_DEPS_TARGET)
            add_library(spdlog_spdlog_libspdlog_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET spdlog_spdlog_libspdlog_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${spdlog_spdlog_libspdlog_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${spdlog_spdlog_libspdlog_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${spdlog_spdlog_libspdlog_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'spdlog_spdlog_libspdlog_DEPS_TARGET' to all of them
        conan_package_library_targets("${spdlog_spdlog_libspdlog_LIBS_RELEASE}"
                                      "${spdlog_spdlog_libspdlog_LIB_DIRS_RELEASE}"
                                      spdlog_spdlog_libspdlog_DEPS_TARGET
                                      spdlog_spdlog_libspdlog_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "spdlog_spdlog_libspdlog")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET spdlog::libspdlog
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${spdlog_spdlog_libspdlog_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${spdlog_spdlog_libspdlog_LIBRARIES_TARGETS}>
                     APPEND)

        if("${spdlog_spdlog_libspdlog_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET spdlog::libspdlog
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         spdlog_spdlog_libspdlog_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET spdlog::libspdlog PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${spdlog_spdlog_libspdlog_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET spdlog::libspdlog PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${spdlog_spdlog_libspdlog_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET spdlog::libspdlog PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${spdlog_spdlog_libspdlog_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET spdlog::libspdlog PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${spdlog_spdlog_libspdlog_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## AGGREGATED GLOBAL TARGET WITH THE COMPONENTS #####################
    set_property(TARGET spdlog::spdlog_header_only PROPERTY INTERFACE_LINK_LIBRARIES spdlog::libspdlog APPEND)

########## For the modules (FindXXX)
set(spdlog_LIBRARIES_RELEASE spdlog::spdlog_header_only)
