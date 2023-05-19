# Avoid multiple calls to find_package to append duplicated properties to the targets
include_guard()########### VARIABLES #######################################################################
#############################################################################################
set(http_parser_FRAMEWORKS_FOUND_RELEASE "") # Will be filled later
conan_find_apple_frameworks(http_parser_FRAMEWORKS_FOUND_RELEASE "${http_parser_FRAMEWORKS_RELEASE}" "${http_parser_FRAMEWORK_DIRS_RELEASE}")

set(http_parser_LIBRARIES_TARGETS "") # Will be filled later


######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
if(NOT TARGET http_parser_DEPS_TARGET)
    add_library(http_parser_DEPS_TARGET INTERFACE IMPORTED)
endif()

set_property(TARGET http_parser_DEPS_TARGET
             PROPERTY INTERFACE_LINK_LIBRARIES
             $<$<CONFIG:Release>:${http_parser_FRAMEWORKS_FOUND_RELEASE}>
             $<$<CONFIG:Release>:${http_parser_SYSTEM_LIBS_RELEASE}>
             $<$<CONFIG:Release>:>
             APPEND)

####### Find the libraries declared in cpp_info.libs, create an IMPORTED target for each one and link the
####### http_parser_DEPS_TARGET to all of them
conan_package_library_targets("${http_parser_LIBS_RELEASE}"    # libraries
                              "${http_parser_LIB_DIRS_RELEASE}" # package_libdir
                              http_parser_DEPS_TARGET
                              http_parser_LIBRARIES_TARGETS  # out_libraries_targets
                              "_RELEASE"
                              "http_parser")    # package_name

# FIXME: What is the result of this for multi-config? All configs adding themselves to path?
set(CMAKE_MODULE_PATH ${http_parser_BUILD_DIRS_RELEASE} ${CMAKE_MODULE_PATH})

########## GLOBAL TARGET PROPERTIES Release ########################################
    set_property(TARGET http_parser::http_parser
                 PROPERTY INTERFACE_LINK_LIBRARIES
                 $<$<CONFIG:Release>:${http_parser_OBJECTS_RELEASE}>
                 $<$<CONFIG:Release>:${http_parser_LIBRARIES_TARGETS}>
                 APPEND)

    if("${http_parser_LIBS_RELEASE}" STREQUAL "")
        # If the package is not declaring any "cpp_info.libs" the package deps, system libs,
        # frameworks etc are not linked to the imported targets and we need to do it to the
        # global target
        set_property(TARGET http_parser::http_parser
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     http_parser_DEPS_TARGET
                     APPEND)
    endif()

    set_property(TARGET http_parser::http_parser
                 PROPERTY INTERFACE_LINK_OPTIONS
                 $<$<CONFIG:Release>:${http_parser_LINKER_FLAGS_RELEASE}> APPEND)
    set_property(TARGET http_parser::http_parser
                 PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                 $<$<CONFIG:Release>:${http_parser_INCLUDE_DIRS_RELEASE}> APPEND)
    set_property(TARGET http_parser::http_parser
                 PROPERTY INTERFACE_COMPILE_DEFINITIONS
                 $<$<CONFIG:Release>:${http_parser_COMPILE_DEFINITIONS_RELEASE}> APPEND)
    set_property(TARGET http_parser::http_parser
                 PROPERTY INTERFACE_COMPILE_OPTIONS
                 $<$<CONFIG:Release>:${http_parser_COMPILE_OPTIONS_RELEASE}> APPEND)

########## For the modules (FindXXX)
set(http_parser_LIBRARIES_RELEASE http_parser::http_parser)
