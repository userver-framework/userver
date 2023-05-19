# Avoid multiple calls to find_package to append duplicated properties to the targets
include_guard()########### VARIABLES #######################################################################
#############################################################################################
set(libnghttp2_FRAMEWORKS_FOUND_RELEASE "") # Will be filled later
conan_find_apple_frameworks(libnghttp2_FRAMEWORKS_FOUND_RELEASE "${libnghttp2_FRAMEWORKS_RELEASE}" "${libnghttp2_FRAMEWORK_DIRS_RELEASE}")

set(libnghttp2_LIBRARIES_TARGETS "") # Will be filled later


######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
if(NOT TARGET libnghttp2_DEPS_TARGET)
    add_library(libnghttp2_DEPS_TARGET INTERFACE IMPORTED)
endif()

set_property(TARGET libnghttp2_DEPS_TARGET
             PROPERTY INTERFACE_LINK_LIBRARIES
             $<$<CONFIG:Release>:${libnghttp2_FRAMEWORKS_FOUND_RELEASE}>
             $<$<CONFIG:Release>:${libnghttp2_SYSTEM_LIBS_RELEASE}>
             $<$<CONFIG:Release>:>
             APPEND)

####### Find the libraries declared in cpp_info.libs, create an IMPORTED target for each one and link the
####### libnghttp2_DEPS_TARGET to all of them
conan_package_library_targets("${libnghttp2_LIBS_RELEASE}"    # libraries
                              "${libnghttp2_LIB_DIRS_RELEASE}" # package_libdir
                              libnghttp2_DEPS_TARGET
                              libnghttp2_LIBRARIES_TARGETS  # out_libraries_targets
                              "_RELEASE"
                              "libnghttp2")    # package_name

# FIXME: What is the result of this for multi-config? All configs adding themselves to path?
set(CMAKE_MODULE_PATH ${libnghttp2_BUILD_DIRS_RELEASE} ${CMAKE_MODULE_PATH})

########## COMPONENTS TARGET PROPERTIES Release ########################################

    ########## COMPONENT libnghttp2::nghttp2 #############

        set(libnghttp2_libnghttp2_nghttp2_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(libnghttp2_libnghttp2_nghttp2_FRAMEWORKS_FOUND_RELEASE "${libnghttp2_libnghttp2_nghttp2_FRAMEWORKS_RELEASE}" "${libnghttp2_libnghttp2_nghttp2_FRAMEWORK_DIRS_RELEASE}")

        set(libnghttp2_libnghttp2_nghttp2_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET libnghttp2_libnghttp2_nghttp2_DEPS_TARGET)
            add_library(libnghttp2_libnghttp2_nghttp2_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET libnghttp2_libnghttp2_nghttp2_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${libnghttp2_libnghttp2_nghttp2_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${libnghttp2_libnghttp2_nghttp2_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${libnghttp2_libnghttp2_nghttp2_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'libnghttp2_libnghttp2_nghttp2_DEPS_TARGET' to all of them
        conan_package_library_targets("${libnghttp2_libnghttp2_nghttp2_LIBS_RELEASE}"
                                      "${libnghttp2_libnghttp2_nghttp2_LIB_DIRS_RELEASE}"
                                      libnghttp2_libnghttp2_nghttp2_DEPS_TARGET
                                      libnghttp2_libnghttp2_nghttp2_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "libnghttp2_libnghttp2_nghttp2")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET libnghttp2::nghttp2
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${libnghttp2_libnghttp2_nghttp2_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${libnghttp2_libnghttp2_nghttp2_LIBRARIES_TARGETS}>
                     APPEND)

        if("${libnghttp2_libnghttp2_nghttp2_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET libnghttp2::nghttp2
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         libnghttp2_libnghttp2_nghttp2_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET libnghttp2::nghttp2 PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${libnghttp2_libnghttp2_nghttp2_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET libnghttp2::nghttp2 PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${libnghttp2_libnghttp2_nghttp2_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET libnghttp2::nghttp2 PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${libnghttp2_libnghttp2_nghttp2_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET libnghttp2::nghttp2 PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${libnghttp2_libnghttp2_nghttp2_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## AGGREGATED GLOBAL TARGET WITH THE COMPONENTS #####################
    set_property(TARGET libnghttp2::libnghttp2 PROPERTY INTERFACE_LINK_LIBRARIES libnghttp2::nghttp2 APPEND)

########## For the modules (FindXXX)
set(libnghttp2_LIBRARIES_RELEASE libnghttp2::libnghttp2)
