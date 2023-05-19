# Avoid multiple calls to find_package to append duplicated properties to the targets
include_guard()########### VARIABLES #######################################################################
#############################################################################################
set(libpq_FRAMEWORKS_FOUND_RELEASE "") # Will be filled later
conan_find_apple_frameworks(libpq_FRAMEWORKS_FOUND_RELEASE "${libpq_FRAMEWORKS_RELEASE}" "${libpq_FRAMEWORK_DIRS_RELEASE}")

set(libpq_LIBRARIES_TARGETS "") # Will be filled later


######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
if(NOT TARGET libpq_DEPS_TARGET)
    add_library(libpq_DEPS_TARGET INTERFACE IMPORTED)
endif()

set_property(TARGET libpq_DEPS_TARGET
             PROPERTY INTERFACE_LINK_LIBRARIES
             $<$<CONFIG:Release>:${libpq_FRAMEWORKS_FOUND_RELEASE}>
             $<$<CONFIG:Release>:${libpq_SYSTEM_LIBS_RELEASE}>
             $<$<CONFIG:Release>:libpq::pgcommon;libpq::pgport>
             APPEND)

####### Find the libraries declared in cpp_info.libs, create an IMPORTED target for each one and link the
####### libpq_DEPS_TARGET to all of them
conan_package_library_targets("${libpq_LIBS_RELEASE}"    # libraries
                              "${libpq_LIB_DIRS_RELEASE}" # package_libdir
                              libpq_DEPS_TARGET
                              libpq_LIBRARIES_TARGETS  # out_libraries_targets
                              "_RELEASE"
                              "libpq")    # package_name

# FIXME: What is the result of this for multi-config? All configs adding themselves to path?
set(CMAKE_MODULE_PATH ${libpq_BUILD_DIRS_RELEASE} ${CMAKE_MODULE_PATH})

########## COMPONENTS TARGET PROPERTIES Release ########################################

    ########## COMPONENT libpq::pq #############

        set(libpq_libpq_pq_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(libpq_libpq_pq_FRAMEWORKS_FOUND_RELEASE "${libpq_libpq_pq_FRAMEWORKS_RELEASE}" "${libpq_libpq_pq_FRAMEWORK_DIRS_RELEASE}")

        set(libpq_libpq_pq_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET libpq_libpq_pq_DEPS_TARGET)
            add_library(libpq_libpq_pq_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET libpq_libpq_pq_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${libpq_libpq_pq_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${libpq_libpq_pq_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${libpq_libpq_pq_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'libpq_libpq_pq_DEPS_TARGET' to all of them
        conan_package_library_targets("${libpq_libpq_pq_LIBS_RELEASE}"
                                      "${libpq_libpq_pq_LIB_DIRS_RELEASE}"
                                      libpq_libpq_pq_DEPS_TARGET
                                      libpq_libpq_pq_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "libpq_libpq_pq")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET libpq::pq
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${libpq_libpq_pq_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${libpq_libpq_pq_LIBRARIES_TARGETS}>
                     APPEND)

        if("${libpq_libpq_pq_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET libpq::pq
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         libpq_libpq_pq_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET libpq::pq PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${libpq_libpq_pq_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET libpq::pq PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${libpq_libpq_pq_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET libpq::pq PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${libpq_libpq_pq_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET libpq::pq PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${libpq_libpq_pq_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT libpq::pgcommon #############

        set(libpq_libpq_pgcommon_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(libpq_libpq_pgcommon_FRAMEWORKS_FOUND_RELEASE "${libpq_libpq_pgcommon_FRAMEWORKS_RELEASE}" "${libpq_libpq_pgcommon_FRAMEWORK_DIRS_RELEASE}")

        set(libpq_libpq_pgcommon_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET libpq_libpq_pgcommon_DEPS_TARGET)
            add_library(libpq_libpq_pgcommon_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET libpq_libpq_pgcommon_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${libpq_libpq_pgcommon_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${libpq_libpq_pgcommon_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${libpq_libpq_pgcommon_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'libpq_libpq_pgcommon_DEPS_TARGET' to all of them
        conan_package_library_targets("${libpq_libpq_pgcommon_LIBS_RELEASE}"
                                      "${libpq_libpq_pgcommon_LIB_DIRS_RELEASE}"
                                      libpq_libpq_pgcommon_DEPS_TARGET
                                      libpq_libpq_pgcommon_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "libpq_libpq_pgcommon")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET libpq::pgcommon
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${libpq_libpq_pgcommon_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${libpq_libpq_pgcommon_LIBRARIES_TARGETS}>
                     APPEND)

        if("${libpq_libpq_pgcommon_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET libpq::pgcommon
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         libpq_libpq_pgcommon_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET libpq::pgcommon PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${libpq_libpq_pgcommon_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET libpq::pgcommon PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${libpq_libpq_pgcommon_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET libpq::pgcommon PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${libpq_libpq_pgcommon_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET libpq::pgcommon PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${libpq_libpq_pgcommon_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT libpq::pgport #############

        set(libpq_libpq_pgport_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(libpq_libpq_pgport_FRAMEWORKS_FOUND_RELEASE "${libpq_libpq_pgport_FRAMEWORKS_RELEASE}" "${libpq_libpq_pgport_FRAMEWORK_DIRS_RELEASE}")

        set(libpq_libpq_pgport_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET libpq_libpq_pgport_DEPS_TARGET)
            add_library(libpq_libpq_pgport_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET libpq_libpq_pgport_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${libpq_libpq_pgport_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${libpq_libpq_pgport_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${libpq_libpq_pgport_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'libpq_libpq_pgport_DEPS_TARGET' to all of them
        conan_package_library_targets("${libpq_libpq_pgport_LIBS_RELEASE}"
                                      "${libpq_libpq_pgport_LIB_DIRS_RELEASE}"
                                      libpq_libpq_pgport_DEPS_TARGET
                                      libpq_libpq_pgport_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "libpq_libpq_pgport")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET libpq::pgport
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${libpq_libpq_pgport_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${libpq_libpq_pgport_LIBRARIES_TARGETS}>
                     APPEND)

        if("${libpq_libpq_pgport_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET libpq::pgport
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         libpq_libpq_pgport_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET libpq::pgport PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${libpq_libpq_pgport_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET libpq::pgport PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${libpq_libpq_pgport_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET libpq::pgport PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${libpq_libpq_pgport_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET libpq::pgport PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${libpq_libpq_pgport_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## AGGREGATED GLOBAL TARGET WITH THE COMPONENTS #####################
    set_property(TARGET PostgreSQL::PostgreSQL PROPERTY INTERFACE_LINK_LIBRARIES libpq::pq APPEND)
    set_property(TARGET PostgreSQL::PostgreSQL PROPERTY INTERFACE_LINK_LIBRARIES libpq::pgcommon APPEND)
    set_property(TARGET PostgreSQL::PostgreSQL PROPERTY INTERFACE_LINK_LIBRARIES libpq::pgport APPEND)

########## For the modules (FindXXX)
set(libpq_LIBRARIES_RELEASE PostgreSQL::PostgreSQL)
