########## MACROS ###########################################################################
#############################################################################################

# Requires CMake > 3.15
if(${CMAKE_VERSION} VERSION_LESS "3.15")
    message(FATAL_ERROR "The 'CMakeDeps' generator only works with CMake >= 3.15")
endif()

if(concurrentqueue_FIND_QUIETLY)
    set(concurrentqueue_MESSAGE_MODE VERBOSE)
else()
    set(concurrentqueue_MESSAGE_MODE STATUS)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/cmakedeps_macros.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/concurrentqueueTargets.cmake)
include(CMakeFindDependencyMacro)

check_build_type_defined()

foreach(_DEPENDENCY ${concurrentqueue_FIND_DEPENDENCY_NAMES} )
    # Check that we have not already called a find_package with the transitive dependency
    if(NOT ${_DEPENDENCY}_FOUND)
        find_dependency(${_DEPENDENCY} REQUIRED ${${_DEPENDENCY}_FIND_MODE})
    endif()
endforeach()

set(concurrentqueue_VERSION_STRING "1.0.3")
set(concurrentqueue_INCLUDE_DIRS ${concurrentqueue_INCLUDE_DIRS_RELEASE} )
set(concurrentqueue_INCLUDE_DIR ${concurrentqueue_INCLUDE_DIRS_RELEASE} )
set(concurrentqueue_LIBRARIES ${concurrentqueue_LIBRARIES_RELEASE} )
set(concurrentqueue_DEFINITIONS ${concurrentqueue_DEFINITIONS_RELEASE} )

# Only the first installed configuration is included to avoid the collision
foreach(_BUILD_MODULE ${concurrentqueue_BUILD_MODULES_PATHS_RELEASE} )
    message(${concurrentqueue_MESSAGE_MODE} "Conan: Including build module from '${_BUILD_MODULE}'")
    include(${_BUILD_MODULE})
endforeach()


