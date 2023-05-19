########## MACROS ###########################################################################
#############################################################################################

# Requires CMake > 3.15
if(${CMAKE_VERSION} VERSION_LESS "3.15")
    message(FATAL_ERROR "The 'CMakeDeps' generator only works with CMake >= 3.15")
endif()

if(fmt_FIND_QUIETLY)
    set(fmt_MESSAGE_MODE VERBOSE)
else()
    set(fmt_MESSAGE_MODE STATUS)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/cmakedeps_macros.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/fmtTargets.cmake)
include(CMakeFindDependencyMacro)

check_build_type_defined()

foreach(_DEPENDENCY ${fmt_FIND_DEPENDENCY_NAMES} )
    # Check that we have not already called a find_package with the transitive dependency
    if(NOT ${_DEPENDENCY}_FOUND)
        find_dependency(${_DEPENDENCY} REQUIRED ${${_DEPENDENCY}_FIND_MODE})
    endif()
endforeach()

set(fmt_VERSION_STRING "8.1.1")
set(fmt_INCLUDE_DIRS ${fmt_INCLUDE_DIRS_RELEASE} )
set(fmt_INCLUDE_DIR ${fmt_INCLUDE_DIRS_RELEASE} )
set(fmt_LIBRARIES ${fmt_LIBRARIES_RELEASE} )
set(fmt_DEFINITIONS ${fmt_DEFINITIONS_RELEASE} )

# Only the first installed configuration is included to avoid the collision
foreach(_BUILD_MODULE ${fmt_BUILD_MODULES_PATHS_RELEASE} )
    message(${fmt_MESSAGE_MODE} "Conan: Including build module from '${_BUILD_MODULE}'")
    include(${_BUILD_MODULE})
endforeach()


