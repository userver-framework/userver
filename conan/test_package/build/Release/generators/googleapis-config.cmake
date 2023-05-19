########## MACROS ###########################################################################
#############################################################################################

# Requires CMake > 3.15
if(${CMAKE_VERSION} VERSION_LESS "3.15")
    message(FATAL_ERROR "The 'CMakeDeps' generator only works with CMake >= 3.15")
endif()

if(googleapis_FIND_QUIETLY)
    set(googleapis_MESSAGE_MODE VERBOSE)
else()
    set(googleapis_MESSAGE_MODE STATUS)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/cmakedeps_macros.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/googleapisTargets.cmake)
include(CMakeFindDependencyMacro)

check_build_type_defined()

foreach(_DEPENDENCY ${googleapis_FIND_DEPENDENCY_NAMES} )
    # Check that we have not already called a find_package with the transitive dependency
    if(NOT ${_DEPENDENCY}_FOUND)
        find_dependency(${_DEPENDENCY} REQUIRED ${${_DEPENDENCY}_FIND_MODE})
    endif()
endforeach()

set(googleapis_VERSION_STRING "cci.20221108")
set(googleapis_INCLUDE_DIRS ${googleapis_INCLUDE_DIRS_RELEASE} )
set(googleapis_INCLUDE_DIR ${googleapis_INCLUDE_DIRS_RELEASE} )
set(googleapis_LIBRARIES ${googleapis_LIBRARIES_RELEASE} )
set(googleapis_DEFINITIONS ${googleapis_DEFINITIONS_RELEASE} )

# Only the first installed configuration is included to avoid the collision
foreach(_BUILD_MODULE ${googleapis_BUILD_MODULES_PATHS_RELEASE} )
    message(${googleapis_MESSAGE_MODE} "Conan: Including build module from '${_BUILD_MODULE}'")
    include(${_BUILD_MODULE})
endforeach()


