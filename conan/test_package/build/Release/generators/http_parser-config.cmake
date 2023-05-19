########## MACROS ###########################################################################
#############################################################################################

# Requires CMake > 3.15
if(${CMAKE_VERSION} VERSION_LESS "3.15")
    message(FATAL_ERROR "The 'CMakeDeps' generator only works with CMake >= 3.15")
endif()

if(http_parser_FIND_QUIETLY)
    set(http_parser_MESSAGE_MODE VERBOSE)
else()
    set(http_parser_MESSAGE_MODE STATUS)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/cmakedeps_macros.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/http_parserTargets.cmake)
include(CMakeFindDependencyMacro)

check_build_type_defined()

foreach(_DEPENDENCY ${http_parser_FIND_DEPENDENCY_NAMES} )
    # Check that we have not already called a find_package with the transitive dependency
    if(NOT ${_DEPENDENCY}_FOUND)
        find_dependency(${_DEPENDENCY} REQUIRED ${${_DEPENDENCY}_FIND_MODE})
    endif()
endforeach()

set(http_parser_VERSION_STRING "2.9.4")
set(http_parser_INCLUDE_DIRS ${http_parser_INCLUDE_DIRS_RELEASE} )
set(http_parser_INCLUDE_DIR ${http_parser_INCLUDE_DIRS_RELEASE} )
set(http_parser_LIBRARIES ${http_parser_LIBRARIES_RELEASE} )
set(http_parser_DEFINITIONS ${http_parser_DEFINITIONS_RELEASE} )

# Only the first installed configuration is included to avoid the collision
foreach(_BUILD_MODULE ${http_parser_BUILD_MODULES_PATHS_RELEASE} )
    message(${http_parser_MESSAGE_MODE} "Conan: Including build module from '${_BUILD_MODULE}'")
    include(${_BUILD_MODULE})
endforeach()


