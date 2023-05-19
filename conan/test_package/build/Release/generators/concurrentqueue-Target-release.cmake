# Avoid multiple calls to find_package to append duplicated properties to the targets
include_guard()########### VARIABLES #######################################################################
#############################################################################################
set(concurrentqueue_FRAMEWORKS_FOUND_RELEASE "") # Will be filled later
conan_find_apple_frameworks(concurrentqueue_FRAMEWORKS_FOUND_RELEASE "${concurrentqueue_FRAMEWORKS_RELEASE}" "${concurrentqueue_FRAMEWORK_DIRS_RELEASE}")

set(concurrentqueue_LIBRARIES_TARGETS "") # Will be filled later


######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
if(NOT TARGET concurrentqueue_DEPS_TARGET)
    add_library(concurrentqueue_DEPS_TARGET INTERFACE IMPORTED)
endif()

set_property(TARGET concurrentqueue_DEPS_TARGET
             PROPERTY INTERFACE_LINK_LIBRARIES
             $<$<CONFIG:Release>:${concurrentqueue_FRAMEWORKS_FOUND_RELEASE}>
             $<$<CONFIG:Release>:${concurrentqueue_SYSTEM_LIBS_RELEASE}>
             $<$<CONFIG:Release>:>
             APPEND)

####### Find the libraries declared in cpp_info.libs, create an IMPORTED target for each one and link the
####### concurrentqueue_DEPS_TARGET to all of them
conan_package_library_targets("${concurrentqueue_LIBS_RELEASE}"    # libraries
                              "${concurrentqueue_LIB_DIRS_RELEASE}" # package_libdir
                              concurrentqueue_DEPS_TARGET
                              concurrentqueue_LIBRARIES_TARGETS  # out_libraries_targets
                              "_RELEASE"
                              "concurrentqueue")    # package_name

# FIXME: What is the result of this for multi-config? All configs adding themselves to path?
set(CMAKE_MODULE_PATH ${concurrentqueue_BUILD_DIRS_RELEASE} ${CMAKE_MODULE_PATH})

########## GLOBAL TARGET PROPERTIES Release ########################################
    set_property(TARGET concurrentqueue::concurrentqueue
                 PROPERTY INTERFACE_LINK_LIBRARIES
                 $<$<CONFIG:Release>:${concurrentqueue_OBJECTS_RELEASE}>
                 $<$<CONFIG:Release>:${concurrentqueue_LIBRARIES_TARGETS}>
                 APPEND)

    if("${concurrentqueue_LIBS_RELEASE}" STREQUAL "")
        # If the package is not declaring any "cpp_info.libs" the package deps, system libs,
        # frameworks etc are not linked to the imported targets and we need to do it to the
        # global target
        set_property(TARGET concurrentqueue::concurrentqueue
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     concurrentqueue_DEPS_TARGET
                     APPEND)
    endif()

    set_property(TARGET concurrentqueue::concurrentqueue
                 PROPERTY INTERFACE_LINK_OPTIONS
                 $<$<CONFIG:Release>:${concurrentqueue_LINKER_FLAGS_RELEASE}> APPEND)
    set_property(TARGET concurrentqueue::concurrentqueue
                 PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                 $<$<CONFIG:Release>:${concurrentqueue_INCLUDE_DIRS_RELEASE}> APPEND)
    set_property(TARGET concurrentqueue::concurrentqueue
                 PROPERTY INTERFACE_COMPILE_DEFINITIONS
                 $<$<CONFIG:Release>:${concurrentqueue_COMPILE_DEFINITIONS_RELEASE}> APPEND)
    set_property(TARGET concurrentqueue::concurrentqueue
                 PROPERTY INTERFACE_COMPILE_OPTIONS
                 $<$<CONFIG:Release>:${concurrentqueue_COMPILE_OPTIONS_RELEASE}> APPEND)

########## For the modules (FindXXX)
set(concurrentqueue_LIBRARIES_RELEASE concurrentqueue::concurrentqueue)
