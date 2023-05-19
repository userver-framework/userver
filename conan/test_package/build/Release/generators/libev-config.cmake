########## MACROS ###########################################################################
#############################################################################################

# Requires CMake > 3.15
if(${CMAKE_VERSION} VERSION_LESS "3.15")
    message(FATAL_ERROR "The 'CMakeDeps' generator only works with CMake >= 3.15")
endif()

if(libev_FIND_QUIETLY)
    set(libev_MESSAGE_MODE VERBOSE)
else()
    set(libev_MESSAGE_MODE STATUS)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/cmakedeps_macros.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/libevTargets.cmake)
include(CMakeFindDependencyMacro)

check_build_type_defined()

foreach(_DEPENDENCY ${libev_FIND_DEPENDENCY_NAMES} )
    # Check that we have not already called a find_package with the transitive dependency
    if(NOT ${_DEPENDENCY}_FOUND)
        find_dependency(${_DEPENDENCY} REQUIRED ${${_DEPENDENCY}_FIND_MODE})
    endif()
endforeach()

set(libev_VERSION_STRING "4.33")
set(libev_INCLUDE_DIRS ${libev_INCLUDE_DIRS_RELEASE} )
set(libev_INCLUDE_DIR ${libev_INCLUDE_DIRS_RELEASE} )
set(libev_LIBRARIES ${libev_LIBRARIES_RELEASE} )
set(libev_DEFINITIONS ${libev_DEFINITIONS_RELEASE} )

# Only the first installed configuration is included to avoid the collision
foreach(_BUILD_MODULE ${libev_BUILD_MODULES_PATHS_RELEASE} )
    message(${libev_MESSAGE_MODE} "Conan: Including build module from '${_BUILD_MODULE}'")
    include(${_BUILD_MODULE})
endforeach()


