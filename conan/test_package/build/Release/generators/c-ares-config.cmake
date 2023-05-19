########## MACROS ###########################################################################
#############################################################################################

# Requires CMake > 3.15
if(${CMAKE_VERSION} VERSION_LESS "3.15")
    message(FATAL_ERROR "The 'CMakeDeps' generator only works with CMake >= 3.15")
endif()

if(c-ares_FIND_QUIETLY)
    set(c-ares_MESSAGE_MODE VERBOSE)
else()
    set(c-ares_MESSAGE_MODE STATUS)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/cmakedeps_macros.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/c-aresTargets.cmake)
include(CMakeFindDependencyMacro)

check_build_type_defined()

foreach(_DEPENDENCY ${c-ares_FIND_DEPENDENCY_NAMES} )
    # Check that we have not already called a find_package with the transitive dependency
    if(NOT ${_DEPENDENCY}_FOUND)
        find_dependency(${_DEPENDENCY} REQUIRED ${${_DEPENDENCY}_FIND_MODE})
    endif()
endforeach()

set(c-ares_VERSION_STRING "1.18.1")
set(c-ares_INCLUDE_DIRS ${c-ares_INCLUDE_DIRS_RELEASE} )
set(c-ares_INCLUDE_DIR ${c-ares_INCLUDE_DIRS_RELEASE} )
set(c-ares_LIBRARIES ${c-ares_LIBRARIES_RELEASE} )
set(c-ares_DEFINITIONS ${c-ares_DEFINITIONS_RELEASE} )

# Only the first installed configuration is included to avoid the collision
foreach(_BUILD_MODULE ${c-ares_BUILD_MODULES_PATHS_RELEASE} )
    message(${c-ares_MESSAGE_MODE} "Conan: Including build module from '${_BUILD_MODULE}'")
    include(${_BUILD_MODULE})
endforeach()


