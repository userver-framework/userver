########## MACROS ###########################################################################
#############################################################################################

# Requires CMake > 3.15
if(${CMAKE_VERSION} VERSION_LESS "3.15")
    message(FATAL_ERROR "The 'CMakeDeps' generator only works with CMake >= 3.15")
endif()

if(hiredis_FIND_QUIETLY)
    set(hiredis_MESSAGE_MODE VERBOSE)
else()
    set(hiredis_MESSAGE_MODE STATUS)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/cmakedeps_macros.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/hiredisTargets.cmake)
include(CMakeFindDependencyMacro)

check_build_type_defined()

foreach(_DEPENDENCY ${hiredis_FIND_DEPENDENCY_NAMES} )
    # Check that we have not already called a find_package with the transitive dependency
    if(NOT ${_DEPENDENCY}_FOUND)
        find_dependency(${_DEPENDENCY} REQUIRED ${${_DEPENDENCY}_FIND_MODE})
    endif()
endforeach()

set(hiredis_VERSION_STRING "1.0.2")
set(hiredis_INCLUDE_DIRS ${hiredis_INCLUDE_DIRS_RELEASE} )
set(hiredis_INCLUDE_DIR ${hiredis_INCLUDE_DIRS_RELEASE} )
set(hiredis_LIBRARIES ${hiredis_LIBRARIES_RELEASE} )
set(hiredis_DEFINITIONS ${hiredis_DEFINITIONS_RELEASE} )

# Only the first installed configuration is included to avoid the collision
foreach(_BUILD_MODULE ${hiredis_BUILD_MODULES_PATHS_RELEASE} )
    message(${hiredis_MESSAGE_MODE} "Conan: Including build module from '${_BUILD_MODULE}'")
    include(${_BUILD_MODULE})
endforeach()


