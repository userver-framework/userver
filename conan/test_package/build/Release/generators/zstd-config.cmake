########## MACROS ###########################################################################
#############################################################################################

# Requires CMake > 3.15
if(${CMAKE_VERSION} VERSION_LESS "3.15")
    message(FATAL_ERROR "The 'CMakeDeps' generator only works with CMake >= 3.15")
endif()

if(zstd_FIND_QUIETLY)
    set(zstd_MESSAGE_MODE VERBOSE)
else()
    set(zstd_MESSAGE_MODE STATUS)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/cmakedeps_macros.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/zstdTargets.cmake)
include(CMakeFindDependencyMacro)

check_build_type_defined()

foreach(_DEPENDENCY ${zstd_FIND_DEPENDENCY_NAMES} )
    # Check that we have not already called a find_package with the transitive dependency
    if(NOT ${_DEPENDENCY}_FOUND)
        find_dependency(${_DEPENDENCY} REQUIRED ${${_DEPENDENCY}_FIND_MODE})
    endif()
endforeach()

set(zstd_VERSION_STRING "1.5.4")
set(zstd_INCLUDE_DIRS ${zstd_INCLUDE_DIRS_RELEASE} )
set(zstd_INCLUDE_DIR ${zstd_INCLUDE_DIRS_RELEASE} )
set(zstd_LIBRARIES ${zstd_LIBRARIES_RELEASE} )
set(zstd_DEFINITIONS ${zstd_DEFINITIONS_RELEASE} )

# Only the first installed configuration is included to avoid the collision
foreach(_BUILD_MODULE ${zstd_BUILD_MODULES_PATHS_RELEASE} )
    message(${zstd_MESSAGE_MODE} "Conan: Including build module from '${_BUILD_MODULE}'")
    include(${_BUILD_MODULE})
endforeach()


