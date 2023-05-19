# Avoid multiple calls to find_package to append duplicated properties to the targets
include_guard()########### VARIABLES #######################################################################
#############################################################################################
set(yaml-cpp_FRAMEWORKS_FOUND_RELEASE "") # Will be filled later
conan_find_apple_frameworks(yaml-cpp_FRAMEWORKS_FOUND_RELEASE "${yaml-cpp_FRAMEWORKS_RELEASE}" "${yaml-cpp_FRAMEWORK_DIRS_RELEASE}")

set(yaml-cpp_LIBRARIES_TARGETS "") # Will be filled later


######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
if(NOT TARGET yaml-cpp_DEPS_TARGET)
    add_library(yaml-cpp_DEPS_TARGET INTERFACE IMPORTED)
endif()

set_property(TARGET yaml-cpp_DEPS_TARGET
             PROPERTY INTERFACE_LINK_LIBRARIES
             $<$<CONFIG:Release>:${yaml-cpp_FRAMEWORKS_FOUND_RELEASE}>
             $<$<CONFIG:Release>:${yaml-cpp_SYSTEM_LIBS_RELEASE}>
             $<$<CONFIG:Release>:>
             APPEND)

####### Find the libraries declared in cpp_info.libs, create an IMPORTED target for each one and link the
####### yaml-cpp_DEPS_TARGET to all of them
conan_package_library_targets("${yaml-cpp_LIBS_RELEASE}"    # libraries
                              "${yaml-cpp_LIB_DIRS_RELEASE}" # package_libdir
                              yaml-cpp_DEPS_TARGET
                              yaml-cpp_LIBRARIES_TARGETS  # out_libraries_targets
                              "_RELEASE"
                              "yaml-cpp")    # package_name

# FIXME: What is the result of this for multi-config? All configs adding themselves to path?
set(CMAKE_MODULE_PATH ${yaml-cpp_BUILD_DIRS_RELEASE} ${CMAKE_MODULE_PATH})

########## GLOBAL TARGET PROPERTIES Release ########################################
    set_property(TARGET yaml-cpp
                 PROPERTY INTERFACE_LINK_LIBRARIES
                 $<$<CONFIG:Release>:${yaml-cpp_OBJECTS_RELEASE}>
                 $<$<CONFIG:Release>:${yaml-cpp_LIBRARIES_TARGETS}>
                 APPEND)

    if("${yaml-cpp_LIBS_RELEASE}" STREQUAL "")
        # If the package is not declaring any "cpp_info.libs" the package deps, system libs,
        # frameworks etc are not linked to the imported targets and we need to do it to the
        # global target
        set_property(TARGET yaml-cpp
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     yaml-cpp_DEPS_TARGET
                     APPEND)
    endif()

    set_property(TARGET yaml-cpp
                 PROPERTY INTERFACE_LINK_OPTIONS
                 $<$<CONFIG:Release>:${yaml-cpp_LINKER_FLAGS_RELEASE}> APPEND)
    set_property(TARGET yaml-cpp
                 PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                 $<$<CONFIG:Release>:${yaml-cpp_INCLUDE_DIRS_RELEASE}> APPEND)
    set_property(TARGET yaml-cpp
                 PROPERTY INTERFACE_COMPILE_DEFINITIONS
                 $<$<CONFIG:Release>:${yaml-cpp_COMPILE_DEFINITIONS_RELEASE}> APPEND)
    set_property(TARGET yaml-cpp
                 PROPERTY INTERFACE_COMPILE_OPTIONS
                 $<$<CONFIG:Release>:${yaml-cpp_COMPILE_OPTIONS_RELEASE}> APPEND)

########## For the modules (FindXXX)
set(yaml-cpp_LIBRARIES_RELEASE yaml-cpp)
