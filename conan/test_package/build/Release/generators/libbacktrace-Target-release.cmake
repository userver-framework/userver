# Avoid multiple calls to find_package to append duplicated properties to the targets
include_guard()########### VARIABLES #######################################################################
#############################################################################################
set(libbacktrace_FRAMEWORKS_FOUND_RELEASE "") # Will be filled later
conan_find_apple_frameworks(libbacktrace_FRAMEWORKS_FOUND_RELEASE "${libbacktrace_FRAMEWORKS_RELEASE}" "${libbacktrace_FRAMEWORK_DIRS_RELEASE}")

set(libbacktrace_LIBRARIES_TARGETS "") # Will be filled later


######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
if(NOT TARGET libbacktrace_DEPS_TARGET)
    add_library(libbacktrace_DEPS_TARGET INTERFACE IMPORTED)
endif()

set_property(TARGET libbacktrace_DEPS_TARGET
             PROPERTY INTERFACE_LINK_LIBRARIES
             $<$<CONFIG:Release>:${libbacktrace_FRAMEWORKS_FOUND_RELEASE}>
             $<$<CONFIG:Release>:${libbacktrace_SYSTEM_LIBS_RELEASE}>
             $<$<CONFIG:Release>:>
             APPEND)

####### Find the libraries declared in cpp_info.libs, create an IMPORTED target for each one and link the
####### libbacktrace_DEPS_TARGET to all of them
conan_package_library_targets("${libbacktrace_LIBS_RELEASE}"    # libraries
                              "${libbacktrace_LIB_DIRS_RELEASE}" # package_libdir
                              libbacktrace_DEPS_TARGET
                              libbacktrace_LIBRARIES_TARGETS  # out_libraries_targets
                              "_RELEASE"
                              "libbacktrace")    # package_name

# FIXME: What is the result of this for multi-config? All configs adding themselves to path?
set(CMAKE_MODULE_PATH ${libbacktrace_BUILD_DIRS_RELEASE} ${CMAKE_MODULE_PATH})

########## GLOBAL TARGET PROPERTIES Release ########################################
    set_property(TARGET libbacktrace::libbacktrace
                 PROPERTY INTERFACE_LINK_LIBRARIES
                 $<$<CONFIG:Release>:${libbacktrace_OBJECTS_RELEASE}>
                 $<$<CONFIG:Release>:${libbacktrace_LIBRARIES_TARGETS}>
                 APPEND)

    if("${libbacktrace_LIBS_RELEASE}" STREQUAL "")
        # If the package is not declaring any "cpp_info.libs" the package deps, system libs,
        # frameworks etc are not linked to the imported targets and we need to do it to the
        # global target
        set_property(TARGET libbacktrace::libbacktrace
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     libbacktrace_DEPS_TARGET
                     APPEND)
    endif()

    set_property(TARGET libbacktrace::libbacktrace
                 PROPERTY INTERFACE_LINK_OPTIONS
                 $<$<CONFIG:Release>:${libbacktrace_LINKER_FLAGS_RELEASE}> APPEND)
    set_property(TARGET libbacktrace::libbacktrace
                 PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                 $<$<CONFIG:Release>:${libbacktrace_INCLUDE_DIRS_RELEASE}> APPEND)
    set_property(TARGET libbacktrace::libbacktrace
                 PROPERTY INTERFACE_COMPILE_DEFINITIONS
                 $<$<CONFIG:Release>:${libbacktrace_COMPILE_DEFINITIONS_RELEASE}> APPEND)
    set_property(TARGET libbacktrace::libbacktrace
                 PROPERTY INTERFACE_COMPILE_OPTIONS
                 $<$<CONFIG:Release>:${libbacktrace_COMPILE_OPTIONS_RELEASE}> APPEND)

########## For the modules (FindXXX)
set(libbacktrace_LIBRARIES_RELEASE libbacktrace::libbacktrace)
