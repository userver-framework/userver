# Avoid multiple calls to find_package to append duplicated properties to the targets
include_guard()########### VARIABLES #######################################################################
#############################################################################################
set(libev_FRAMEWORKS_FOUND_RELEASE "") # Will be filled later
conan_find_apple_frameworks(libev_FRAMEWORKS_FOUND_RELEASE "${libev_FRAMEWORKS_RELEASE}" "${libev_FRAMEWORK_DIRS_RELEASE}")

set(libev_LIBRARIES_TARGETS "") # Will be filled later


######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
if(NOT TARGET libev_DEPS_TARGET)
    add_library(libev_DEPS_TARGET INTERFACE IMPORTED)
endif()

set_property(TARGET libev_DEPS_TARGET
             PROPERTY INTERFACE_LINK_LIBRARIES
             $<$<CONFIG:Release>:${libev_FRAMEWORKS_FOUND_RELEASE}>
             $<$<CONFIG:Release>:${libev_SYSTEM_LIBS_RELEASE}>
             $<$<CONFIG:Release>:>
             APPEND)

####### Find the libraries declared in cpp_info.libs, create an IMPORTED target for each one and link the
####### libev_DEPS_TARGET to all of them
conan_package_library_targets("${libev_LIBS_RELEASE}"    # libraries
                              "${libev_LIB_DIRS_RELEASE}" # package_libdir
                              libev_DEPS_TARGET
                              libev_LIBRARIES_TARGETS  # out_libraries_targets
                              "_RELEASE"
                              "libev")    # package_name

# FIXME: What is the result of this for multi-config? All configs adding themselves to path?
set(CMAKE_MODULE_PATH ${libev_BUILD_DIRS_RELEASE} ${CMAKE_MODULE_PATH})

########## GLOBAL TARGET PROPERTIES Release ########################################
    set_property(TARGET libev::libev
                 PROPERTY INTERFACE_LINK_LIBRARIES
                 $<$<CONFIG:Release>:${libev_OBJECTS_RELEASE}>
                 $<$<CONFIG:Release>:${libev_LIBRARIES_TARGETS}>
                 APPEND)

    if("${libev_LIBS_RELEASE}" STREQUAL "")
        # If the package is not declaring any "cpp_info.libs" the package deps, system libs,
        # frameworks etc are not linked to the imported targets and we need to do it to the
        # global target
        set_property(TARGET libev::libev
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     libev_DEPS_TARGET
                     APPEND)
    endif()

    set_property(TARGET libev::libev
                 PROPERTY INTERFACE_LINK_OPTIONS
                 $<$<CONFIG:Release>:${libev_LINKER_FLAGS_RELEASE}> APPEND)
    set_property(TARGET libev::libev
                 PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                 $<$<CONFIG:Release>:${libev_INCLUDE_DIRS_RELEASE}> APPEND)
    set_property(TARGET libev::libev
                 PROPERTY INTERFACE_COMPILE_DEFINITIONS
                 $<$<CONFIG:Release>:${libev_COMPILE_DEFINITIONS_RELEASE}> APPEND)
    set_property(TARGET libev::libev
                 PROPERTY INTERFACE_COMPILE_OPTIONS
                 $<$<CONFIG:Release>:${libev_COMPILE_OPTIONS_RELEASE}> APPEND)

########## For the modules (FindXXX)
set(libev_LIBRARIES_RELEASE libev::libev)
