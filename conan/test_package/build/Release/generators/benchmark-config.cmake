########## MACROS ###########################################################################
#############################################################################################

# Requires CMake > 3.15
if(${CMAKE_VERSION} VERSION_LESS "3.15")
    message(FATAL_ERROR "The 'CMakeDeps' generator only works with CMake >= 3.15")
endif()

if(benchmark_FIND_QUIETLY)
    set(benchmark_MESSAGE_MODE VERBOSE)
else()
    set(benchmark_MESSAGE_MODE STATUS)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/cmakedeps_macros.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/benchmarkTargets.cmake)
include(CMakeFindDependencyMacro)

check_build_type_defined()

foreach(_DEPENDENCY ${benchmark_FIND_DEPENDENCY_NAMES} )
    # Check that we have not already called a find_package with the transitive dependency
    if(NOT ${_DEPENDENCY}_FOUND)
        find_dependency(${_DEPENDENCY} REQUIRED ${${_DEPENDENCY}_FIND_MODE})
    endif()
endforeach()

set(benchmark_VERSION_STRING "1.6.2")
set(benchmark_INCLUDE_DIRS ${benchmark_INCLUDE_DIRS_RELEASE} )
set(benchmark_INCLUDE_DIR ${benchmark_INCLUDE_DIRS_RELEASE} )
set(benchmark_LIBRARIES ${benchmark_LIBRARIES_RELEASE} )
set(benchmark_DEFINITIONS ${benchmark_DEFINITIONS_RELEASE} )

# Only the first installed configuration is included to avoid the collision
foreach(_BUILD_MODULE ${benchmark_BUILD_MODULES_PATHS_RELEASE} )
    message(${benchmark_MESSAGE_MODE} "Conan: Including build module from '${_BUILD_MODULE}'")
    include(${_BUILD_MODULE})
endforeach()


