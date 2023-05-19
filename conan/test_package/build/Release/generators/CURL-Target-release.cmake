# Avoid multiple calls to find_package to append duplicated properties to the targets
include_guard()########### VARIABLES #######################################################################
#############################################################################################
set(libcurl_FRAMEWORKS_FOUND_RELEASE "") # Will be filled later
conan_find_apple_frameworks(libcurl_FRAMEWORKS_FOUND_RELEASE "${libcurl_FRAMEWORKS_RELEASE}" "${libcurl_FRAMEWORK_DIRS_RELEASE}")

set(libcurl_LIBRARIES_TARGETS "") # Will be filled later


######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
if(NOT TARGET libcurl_DEPS_TARGET)
    add_library(libcurl_DEPS_TARGET INTERFACE IMPORTED)
endif()

set_property(TARGET libcurl_DEPS_TARGET
             PROPERTY INTERFACE_LINK_LIBRARIES
             $<$<CONFIG:Release>:${libcurl_FRAMEWORKS_FOUND_RELEASE}>
             $<$<CONFIG:Release>:${libcurl_SYSTEM_LIBS_RELEASE}>
             $<$<CONFIG:Release>:ZLIB::ZLIB>
             APPEND)

####### Find the libraries declared in cpp_info.libs, create an IMPORTED target for each one and link the
####### libcurl_DEPS_TARGET to all of them
conan_package_library_targets("${libcurl_LIBS_RELEASE}"    # libraries
                              "${libcurl_LIB_DIRS_RELEASE}" # package_libdir
                              libcurl_DEPS_TARGET
                              libcurl_LIBRARIES_TARGETS  # out_libraries_targets
                              "_RELEASE"
                              "libcurl")    # package_name

# FIXME: What is the result of this for multi-config? All configs adding themselves to path?
set(CMAKE_MODULE_PATH ${libcurl_BUILD_DIRS_RELEASE} ${CMAKE_MODULE_PATH})

########## COMPONENTS TARGET PROPERTIES Release ########################################

    ########## COMPONENT CURL::libcurl #############

        set(libcurl_CURL_libcurl_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(libcurl_CURL_libcurl_FRAMEWORKS_FOUND_RELEASE "${libcurl_CURL_libcurl_FRAMEWORKS_RELEASE}" "${libcurl_CURL_libcurl_FRAMEWORK_DIRS_RELEASE}")

        set(libcurl_CURL_libcurl_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET libcurl_CURL_libcurl_DEPS_TARGET)
            add_library(libcurl_CURL_libcurl_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET libcurl_CURL_libcurl_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${libcurl_CURL_libcurl_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${libcurl_CURL_libcurl_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${libcurl_CURL_libcurl_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'libcurl_CURL_libcurl_DEPS_TARGET' to all of them
        conan_package_library_targets("${libcurl_CURL_libcurl_LIBS_RELEASE}"
                                      "${libcurl_CURL_libcurl_LIB_DIRS_RELEASE}"
                                      libcurl_CURL_libcurl_DEPS_TARGET
                                      libcurl_CURL_libcurl_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "libcurl_CURL_libcurl")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET CURL::libcurl
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${libcurl_CURL_libcurl_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${libcurl_CURL_libcurl_LIBRARIES_TARGETS}>
                     APPEND)

        if("${libcurl_CURL_libcurl_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET CURL::libcurl
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         libcurl_CURL_libcurl_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET CURL::libcurl PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${libcurl_CURL_libcurl_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET CURL::libcurl PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${libcurl_CURL_libcurl_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET CURL::libcurl PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${libcurl_CURL_libcurl_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET CURL::libcurl PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${libcurl_CURL_libcurl_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## AGGREGATED GLOBAL TARGET WITH THE COMPONENTS #####################
    set_property(TARGET CURL::libcurl PROPERTY INTERFACE_LINK_LIBRARIES CURL::libcurl APPEND)

########## For the modules (FindXXX)
set(libcurl_LIBRARIES_RELEASE CURL::libcurl)
