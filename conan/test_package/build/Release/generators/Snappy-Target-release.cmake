# Avoid multiple calls to find_package to append duplicated properties to the targets
include_guard()########### VARIABLES #######################################################################
#############################################################################################
set(snappy_FRAMEWORKS_FOUND_RELEASE "") # Will be filled later
conan_find_apple_frameworks(snappy_FRAMEWORKS_FOUND_RELEASE "${snappy_FRAMEWORKS_RELEASE}" "${snappy_FRAMEWORK_DIRS_RELEASE}")

set(snappy_LIBRARIES_TARGETS "") # Will be filled later


######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
if(NOT TARGET snappy_DEPS_TARGET)
    add_library(snappy_DEPS_TARGET INTERFACE IMPORTED)
endif()

set_property(TARGET snappy_DEPS_TARGET
             PROPERTY INTERFACE_LINK_LIBRARIES
             $<$<CONFIG:Release>:${snappy_FRAMEWORKS_FOUND_RELEASE}>
             $<$<CONFIG:Release>:${snappy_SYSTEM_LIBS_RELEASE}>
             $<$<CONFIG:Release>:>
             APPEND)

####### Find the libraries declared in cpp_info.libs, create an IMPORTED target for each one and link the
####### snappy_DEPS_TARGET to all of them
conan_package_library_targets("${snappy_LIBS_RELEASE}"    # libraries
                              "${snappy_LIB_DIRS_RELEASE}" # package_libdir
                              snappy_DEPS_TARGET
                              snappy_LIBRARIES_TARGETS  # out_libraries_targets
                              "_RELEASE"
                              "snappy")    # package_name

# FIXME: What is the result of this for multi-config? All configs adding themselves to path?
set(CMAKE_MODULE_PATH ${snappy_BUILD_DIRS_RELEASE} ${CMAKE_MODULE_PATH})

########## COMPONENTS TARGET PROPERTIES Release ########################################

    ########## COMPONENT Snappy::snappy #############

        set(snappy_Snappy_snappy_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(snappy_Snappy_snappy_FRAMEWORKS_FOUND_RELEASE "${snappy_Snappy_snappy_FRAMEWORKS_RELEASE}" "${snappy_Snappy_snappy_FRAMEWORK_DIRS_RELEASE}")

        set(snappy_Snappy_snappy_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET snappy_Snappy_snappy_DEPS_TARGET)
            add_library(snappy_Snappy_snappy_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET snappy_Snappy_snappy_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${snappy_Snappy_snappy_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${snappy_Snappy_snappy_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${snappy_Snappy_snappy_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'snappy_Snappy_snappy_DEPS_TARGET' to all of them
        conan_package_library_targets("${snappy_Snappy_snappy_LIBS_RELEASE}"
                                      "${snappy_Snappy_snappy_LIB_DIRS_RELEASE}"
                                      snappy_Snappy_snappy_DEPS_TARGET
                                      snappy_Snappy_snappy_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "snappy_Snappy_snappy")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET Snappy::snappy
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${snappy_Snappy_snappy_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${snappy_Snappy_snappy_LIBRARIES_TARGETS}>
                     APPEND)

        if("${snappy_Snappy_snappy_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET Snappy::snappy
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         snappy_Snappy_snappy_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET Snappy::snappy PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${snappy_Snappy_snappy_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET Snappy::snappy PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${snappy_Snappy_snappy_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET Snappy::snappy PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${snappy_Snappy_snappy_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET Snappy::snappy PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${snappy_Snappy_snappy_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## AGGREGATED GLOBAL TARGET WITH THE COMPONENTS #####################
    set_property(TARGET Snappy::snappy PROPERTY INTERFACE_LINK_LIBRARIES Snappy::snappy APPEND)

########## For the modules (FindXXX)
set(snappy_LIBRARIES_RELEASE Snappy::snappy)
