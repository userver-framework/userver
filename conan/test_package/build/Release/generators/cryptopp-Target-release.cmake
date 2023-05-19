# Avoid multiple calls to find_package to append duplicated properties to the targets
include_guard()########### VARIABLES #######################################################################
#############################################################################################
set(cryptopp_FRAMEWORKS_FOUND_RELEASE "") # Will be filled later
conan_find_apple_frameworks(cryptopp_FRAMEWORKS_FOUND_RELEASE "${cryptopp_FRAMEWORKS_RELEASE}" "${cryptopp_FRAMEWORK_DIRS_RELEASE}")

set(cryptopp_LIBRARIES_TARGETS "") # Will be filled later


######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
if(NOT TARGET cryptopp_DEPS_TARGET)
    add_library(cryptopp_DEPS_TARGET INTERFACE IMPORTED)
endif()

set_property(TARGET cryptopp_DEPS_TARGET
             PROPERTY INTERFACE_LINK_LIBRARIES
             $<$<CONFIG:Release>:${cryptopp_FRAMEWORKS_FOUND_RELEASE}>
             $<$<CONFIG:Release>:${cryptopp_SYSTEM_LIBS_RELEASE}>
             $<$<CONFIG:Release>:>
             APPEND)

####### Find the libraries declared in cpp_info.libs, create an IMPORTED target for each one and link the
####### cryptopp_DEPS_TARGET to all of them
conan_package_library_targets("${cryptopp_LIBS_RELEASE}"    # libraries
                              "${cryptopp_LIB_DIRS_RELEASE}" # package_libdir
                              cryptopp_DEPS_TARGET
                              cryptopp_LIBRARIES_TARGETS  # out_libraries_targets
                              "_RELEASE"
                              "cryptopp")    # package_name

# FIXME: What is the result of this for multi-config? All configs adding themselves to path?
set(CMAKE_MODULE_PATH ${cryptopp_BUILD_DIRS_RELEASE} ${CMAKE_MODULE_PATH})

########## COMPONENTS TARGET PROPERTIES Release ########################################

    ########## COMPONENT cryptopp::cryptopp #############

        set(cryptopp_cryptopp_cryptopp_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(cryptopp_cryptopp_cryptopp_FRAMEWORKS_FOUND_RELEASE "${cryptopp_cryptopp_cryptopp_FRAMEWORKS_RELEASE}" "${cryptopp_cryptopp_cryptopp_FRAMEWORK_DIRS_RELEASE}")

        set(cryptopp_cryptopp_cryptopp_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET cryptopp_cryptopp_cryptopp_DEPS_TARGET)
            add_library(cryptopp_cryptopp_cryptopp_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET cryptopp_cryptopp_cryptopp_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${cryptopp_cryptopp_cryptopp_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${cryptopp_cryptopp_cryptopp_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${cryptopp_cryptopp_cryptopp_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'cryptopp_cryptopp_cryptopp_DEPS_TARGET' to all of them
        conan_package_library_targets("${cryptopp_cryptopp_cryptopp_LIBS_RELEASE}"
                                      "${cryptopp_cryptopp_cryptopp_LIB_DIRS_RELEASE}"
                                      cryptopp_cryptopp_cryptopp_DEPS_TARGET
                                      cryptopp_cryptopp_cryptopp_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "cryptopp_cryptopp_cryptopp")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET cryptopp::cryptopp
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${cryptopp_cryptopp_cryptopp_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${cryptopp_cryptopp_cryptopp_LIBRARIES_TARGETS}>
                     APPEND)

        if("${cryptopp_cryptopp_cryptopp_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET cryptopp::cryptopp
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         cryptopp_cryptopp_cryptopp_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET cryptopp::cryptopp PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${cryptopp_cryptopp_cryptopp_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET cryptopp::cryptopp PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${cryptopp_cryptopp_cryptopp_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET cryptopp::cryptopp PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${cryptopp_cryptopp_cryptopp_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET cryptopp::cryptopp PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${cryptopp_cryptopp_cryptopp_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## AGGREGATED GLOBAL TARGET WITH THE COMPONENTS #####################
    set_property(TARGET cryptopp::cryptopp PROPERTY INTERFACE_LINK_LIBRARIES cryptopp::cryptopp APPEND)

########## For the modules (FindXXX)
set(cryptopp_LIBRARIES_RELEASE cryptopp::cryptopp)
