# Avoid multiple calls to find_package to append duplicated properties to the targets
include_guard()########### VARIABLES #######################################################################
#############################################################################################
set(amqp-cpp_FRAMEWORKS_FOUND_RELEASE "") # Will be filled later
conan_find_apple_frameworks(amqp-cpp_FRAMEWORKS_FOUND_RELEASE "${amqp-cpp_FRAMEWORKS_RELEASE}" "${amqp-cpp_FRAMEWORK_DIRS_RELEASE}")

set(amqp-cpp_LIBRARIES_TARGETS "") # Will be filled later


######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
if(NOT TARGET amqp-cpp_DEPS_TARGET)
    add_library(amqp-cpp_DEPS_TARGET INTERFACE IMPORTED)
endif()

set_property(TARGET amqp-cpp_DEPS_TARGET
             PROPERTY INTERFACE_LINK_LIBRARIES
             $<$<CONFIG:Release>:${amqp-cpp_FRAMEWORKS_FOUND_RELEASE}>
             $<$<CONFIG:Release>:${amqp-cpp_SYSTEM_LIBS_RELEASE}>
             $<$<CONFIG:Release>:openssl::openssl>
             APPEND)

####### Find the libraries declared in cpp_info.libs, create an IMPORTED target for each one and link the
####### amqp-cpp_DEPS_TARGET to all of them
conan_package_library_targets("${amqp-cpp_LIBS_RELEASE}"    # libraries
                              "${amqp-cpp_LIB_DIRS_RELEASE}" # package_libdir
                              amqp-cpp_DEPS_TARGET
                              amqp-cpp_LIBRARIES_TARGETS  # out_libraries_targets
                              "_RELEASE"
                              "amqp-cpp")    # package_name

# FIXME: What is the result of this for multi-config? All configs adding themselves to path?
set(CMAKE_MODULE_PATH ${amqp-cpp_BUILD_DIRS_RELEASE} ${CMAKE_MODULE_PATH})

########## GLOBAL TARGET PROPERTIES Release ########################################
    set_property(TARGET amqpcpp
                 PROPERTY INTERFACE_LINK_LIBRARIES
                 $<$<CONFIG:Release>:${amqp-cpp_OBJECTS_RELEASE}>
                 $<$<CONFIG:Release>:${amqp-cpp_LIBRARIES_TARGETS}>
                 APPEND)

    if("${amqp-cpp_LIBS_RELEASE}" STREQUAL "")
        # If the package is not declaring any "cpp_info.libs" the package deps, system libs,
        # frameworks etc are not linked to the imported targets and we need to do it to the
        # global target
        set_property(TARGET amqpcpp
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     amqp-cpp_DEPS_TARGET
                     APPEND)
    endif()

    set_property(TARGET amqpcpp
                 PROPERTY INTERFACE_LINK_OPTIONS
                 $<$<CONFIG:Release>:${amqp-cpp_LINKER_FLAGS_RELEASE}> APPEND)
    set_property(TARGET amqpcpp
                 PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                 $<$<CONFIG:Release>:${amqp-cpp_INCLUDE_DIRS_RELEASE}> APPEND)
    set_property(TARGET amqpcpp
                 PROPERTY INTERFACE_COMPILE_DEFINITIONS
                 $<$<CONFIG:Release>:${amqp-cpp_COMPILE_DEFINITIONS_RELEASE}> APPEND)
    set_property(TARGET amqpcpp
                 PROPERTY INTERFACE_COMPILE_OPTIONS
                 $<$<CONFIG:Release>:${amqp-cpp_COMPILE_OPTIONS_RELEASE}> APPEND)

########## For the modules (FindXXX)
set(amqp-cpp_LIBRARIES_RELEASE amqpcpp)
