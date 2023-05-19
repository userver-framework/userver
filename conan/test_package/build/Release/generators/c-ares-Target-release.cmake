# Avoid multiple calls to find_package to append duplicated properties to the targets
include_guard()########### VARIABLES #######################################################################
#############################################################################################
set(c-ares_FRAMEWORKS_FOUND_RELEASE "") # Will be filled later
conan_find_apple_frameworks(c-ares_FRAMEWORKS_FOUND_RELEASE "${c-ares_FRAMEWORKS_RELEASE}" "${c-ares_FRAMEWORK_DIRS_RELEASE}")

set(c-ares_LIBRARIES_TARGETS "") # Will be filled later


######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
if(NOT TARGET c-ares_DEPS_TARGET)
    add_library(c-ares_DEPS_TARGET INTERFACE IMPORTED)
endif()

set_property(TARGET c-ares_DEPS_TARGET
             PROPERTY INTERFACE_LINK_LIBRARIES
             $<$<CONFIG:Release>:${c-ares_FRAMEWORKS_FOUND_RELEASE}>
             $<$<CONFIG:Release>:${c-ares_SYSTEM_LIBS_RELEASE}>
             $<$<CONFIG:Release>:>
             APPEND)

####### Find the libraries declared in cpp_info.libs, create an IMPORTED target for each one and link the
####### c-ares_DEPS_TARGET to all of them
conan_package_library_targets("${c-ares_LIBS_RELEASE}"    # libraries
                              "${c-ares_LIB_DIRS_RELEASE}" # package_libdir
                              c-ares_DEPS_TARGET
                              c-ares_LIBRARIES_TARGETS  # out_libraries_targets
                              "_RELEASE"
                              "c-ares")    # package_name

# FIXME: What is the result of this for multi-config? All configs adding themselves to path?
set(CMAKE_MODULE_PATH ${c-ares_BUILD_DIRS_RELEASE} ${CMAKE_MODULE_PATH})

########## COMPONENTS TARGET PROPERTIES Release ########################################

    ########## COMPONENT c-ares::cares #############

        set(c-ares_c-ares_cares_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(c-ares_c-ares_cares_FRAMEWORKS_FOUND_RELEASE "${c-ares_c-ares_cares_FRAMEWORKS_RELEASE}" "${c-ares_c-ares_cares_FRAMEWORK_DIRS_RELEASE}")

        set(c-ares_c-ares_cares_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET c-ares_c-ares_cares_DEPS_TARGET)
            add_library(c-ares_c-ares_cares_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET c-ares_c-ares_cares_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${c-ares_c-ares_cares_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${c-ares_c-ares_cares_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${c-ares_c-ares_cares_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'c-ares_c-ares_cares_DEPS_TARGET' to all of them
        conan_package_library_targets("${c-ares_c-ares_cares_LIBS_RELEASE}"
                                      "${c-ares_c-ares_cares_LIB_DIRS_RELEASE}"
                                      c-ares_c-ares_cares_DEPS_TARGET
                                      c-ares_c-ares_cares_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "c-ares_c-ares_cares")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET c-ares::cares
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${c-ares_c-ares_cares_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${c-ares_c-ares_cares_LIBRARIES_TARGETS}>
                     APPEND)

        if("${c-ares_c-ares_cares_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET c-ares::cares
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         c-ares_c-ares_cares_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET c-ares::cares PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${c-ares_c-ares_cares_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET c-ares::cares PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${c-ares_c-ares_cares_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET c-ares::cares PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${c-ares_c-ares_cares_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET c-ares::cares PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${c-ares_c-ares_cares_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## AGGREGATED GLOBAL TARGET WITH THE COMPONENTS #####################
    set_property(TARGET c-ares::cares PROPERTY INTERFACE_LINK_LIBRARIES c-ares::cares APPEND)

########## For the modules (FindXXX)
set(c-ares_LIBRARIES_RELEASE c-ares::cares)
