# Avoid multiple calls to find_package to append duplicated properties to the targets
include_guard()########### VARIABLES #######################################################################
#############################################################################################
set(libiconv_FRAMEWORKS_FOUND_RELEASE "") # Will be filled later
conan_find_apple_frameworks(libiconv_FRAMEWORKS_FOUND_RELEASE "${libiconv_FRAMEWORKS_RELEASE}" "${libiconv_FRAMEWORK_DIRS_RELEASE}")

set(libiconv_LIBRARIES_TARGETS "") # Will be filled later


######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
if(NOT TARGET libiconv_DEPS_TARGET)
    add_library(libiconv_DEPS_TARGET INTERFACE IMPORTED)
endif()

set_property(TARGET libiconv_DEPS_TARGET
             PROPERTY INTERFACE_LINK_LIBRARIES
             $<$<CONFIG:Release>:${libiconv_FRAMEWORKS_FOUND_RELEASE}>
             $<$<CONFIG:Release>:${libiconv_SYSTEM_LIBS_RELEASE}>
             $<$<CONFIG:Release>:>
             APPEND)

####### Find the libraries declared in cpp_info.libs, create an IMPORTED target for each one and link the
####### libiconv_DEPS_TARGET to all of them
conan_package_library_targets("${libiconv_LIBS_RELEASE}"    # libraries
                              "${libiconv_LIB_DIRS_RELEASE}" # package_libdir
                              libiconv_DEPS_TARGET
                              libiconv_LIBRARIES_TARGETS  # out_libraries_targets
                              "_RELEASE"
                              "libiconv")    # package_name

# FIXME: What is the result of this for multi-config? All configs adding themselves to path?
set(CMAKE_MODULE_PATH ${libiconv_BUILD_DIRS_RELEASE} ${CMAKE_MODULE_PATH})

########## GLOBAL TARGET PROPERTIES Release ########################################
    set_property(TARGET Iconv::Iconv
                 PROPERTY INTERFACE_LINK_LIBRARIES
                 $<$<CONFIG:Release>:${libiconv_OBJECTS_RELEASE}>
                 $<$<CONFIG:Release>:${libiconv_LIBRARIES_TARGETS}>
                 APPEND)

    if("${libiconv_LIBS_RELEASE}" STREQUAL "")
        # If the package is not declaring any "cpp_info.libs" the package deps, system libs,
        # frameworks etc are not linked to the imported targets and we need to do it to the
        # global target
        set_property(TARGET Iconv::Iconv
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     libiconv_DEPS_TARGET
                     APPEND)
    endif()

    set_property(TARGET Iconv::Iconv
                 PROPERTY INTERFACE_LINK_OPTIONS
                 $<$<CONFIG:Release>:${libiconv_LINKER_FLAGS_RELEASE}> APPEND)
    set_property(TARGET Iconv::Iconv
                 PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                 $<$<CONFIG:Release>:${libiconv_INCLUDE_DIRS_RELEASE}> APPEND)
    set_property(TARGET Iconv::Iconv
                 PROPERTY INTERFACE_COMPILE_DEFINITIONS
                 $<$<CONFIG:Release>:${libiconv_COMPILE_DEFINITIONS_RELEASE}> APPEND)
    set_property(TARGET Iconv::Iconv
                 PROPERTY INTERFACE_COMPILE_OPTIONS
                 $<$<CONFIG:Release>:${libiconv_COMPILE_OPTIONS_RELEASE}> APPEND)

########## For the modules (FindXXX)
set(libiconv_LIBRARIES_RELEASE Iconv::Iconv)
