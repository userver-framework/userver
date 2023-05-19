########## MACROS ###########################################################################
#############################################################################################

# Requires CMake > 3.15
if(${CMAKE_VERSION} VERSION_LESS "3.15")
    message(FATAL_ERROR "The 'CMakeDeps' generator only works with CMake >= 3.15")
endif()

if(absl_FIND_QUIETLY)
    set(absl_MESSAGE_MODE VERBOSE)
else()
    set(absl_MESSAGE_MODE STATUS)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/cmakedeps_macros.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/abslTargets.cmake)
include(CMakeFindDependencyMacro)

check_build_type_defined()

foreach(_DEPENDENCY ${abseil_FIND_DEPENDENCY_NAMES} )
    # Check that we have not already called a find_package with the transitive dependency
    if(NOT ${_DEPENDENCY}_FOUND)
        find_dependency(${_DEPENDENCY} REQUIRED ${${_DEPENDENCY}_FIND_MODE})
    endif()
endforeach()

set(absl_VERSION_STRING "20220623.0")
set(absl_INCLUDE_DIRS ${abseil_INCLUDE_DIRS_RELEASE} )
set(absl_INCLUDE_DIR ${abseil_INCLUDE_DIRS_RELEASE} )
set(absl_LIBRARIES ${abseil_LIBRARIES_RELEASE} )
set(absl_DEFINITIONS ${abseil_DEFINITIONS_RELEASE} )

# Only the first installed configuration is included to avoid the collision
foreach(_BUILD_MODULE ${abseil_BUILD_MODULES_PATHS_RELEASE} )
    message(${absl_MESSAGE_MODE} "Conan: Including build module from '${_BUILD_MODULE}'")
    include(${_BUILD_MODULE})
endforeach()


