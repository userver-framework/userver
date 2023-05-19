# Avoid multiple calls to find_package to append duplicated properties to the targets
include_guard()########### VARIABLES #######################################################################
#############################################################################################
set(fmt_FRAMEWORKS_FOUND_RELEASE "") # Will be filled later
conan_find_apple_frameworks(fmt_FRAMEWORKS_FOUND_RELEASE "${fmt_FRAMEWORKS_RELEASE}" "${fmt_FRAMEWORK_DIRS_RELEASE}")

set(fmt_LIBRARIES_TARGETS "") # Will be filled later


######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
if(NOT TARGET fmt_DEPS_TARGET)
    add_library(fmt_DEPS_TARGET INTERFACE IMPORTED)
endif()

set_property(TARGET fmt_DEPS_TARGET
             PROPERTY INTERFACE_LINK_LIBRARIES
             $<$<CONFIG:Release>:${fmt_FRAMEWORKS_FOUND_RELEASE}>
             $<$<CONFIG:Release>:${fmt_SYSTEM_LIBS_RELEASE}>
             $<$<CONFIG:Release>:>
             APPEND)

####### Find the libraries declared in cpp_info.libs, create an IMPORTED target for each one and link the
####### fmt_DEPS_TARGET to all of them
conan_package_library_targets("${fmt_LIBS_RELEASE}"    # libraries
                              "${fmt_LIB_DIRS_RELEASE}" # package_libdir
                              fmt_DEPS_TARGET
                              fmt_LIBRARIES_TARGETS  # out_libraries_targets
                              "_RELEASE"
                              "fmt")    # package_name

# FIXME: What is the result of this for multi-config? All configs adding themselves to path?
set(CMAKE_MODULE_PATH ${fmt_BUILD_DIRS_RELEASE} ${CMAKE_MODULE_PATH})

########## COMPONENTS TARGET PROPERTIES Release ########################################

    ########## COMPONENT fmt::fmt #############

        set(fmt_fmt_fmt_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(fmt_fmt_fmt_FRAMEWORKS_FOUND_RELEASE "${fmt_fmt_fmt_FRAMEWORKS_RELEASE}" "${fmt_fmt_fmt_FRAMEWORK_DIRS_RELEASE}")

        set(fmt_fmt_fmt_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET fmt_fmt_fmt_DEPS_TARGET)
            add_library(fmt_fmt_fmt_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET fmt_fmt_fmt_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${fmt_fmt_fmt_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${fmt_fmt_fmt_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${fmt_fmt_fmt_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'fmt_fmt_fmt_DEPS_TARGET' to all of them
        conan_package_library_targets("${fmt_fmt_fmt_LIBS_RELEASE}"
                                      "${fmt_fmt_fmt_LIB_DIRS_RELEASE}"
                                      fmt_fmt_fmt_DEPS_TARGET
                                      fmt_fmt_fmt_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "fmt_fmt_fmt")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET fmt::fmt
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${fmt_fmt_fmt_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${fmt_fmt_fmt_LIBRARIES_TARGETS}>
                     APPEND)

        if("${fmt_fmt_fmt_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET fmt::fmt
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         fmt_fmt_fmt_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET fmt::fmt PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${fmt_fmt_fmt_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET fmt::fmt PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${fmt_fmt_fmt_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET fmt::fmt PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${fmt_fmt_fmt_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET fmt::fmt PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${fmt_fmt_fmt_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## AGGREGATED GLOBAL TARGET WITH THE COMPONENTS #####################
    set_property(TARGET fmt::fmt PROPERTY INTERFACE_LINK_LIBRARIES fmt::fmt APPEND)

########## For the modules (FindXXX)
set(fmt_LIBRARIES_RELEASE fmt::fmt)
