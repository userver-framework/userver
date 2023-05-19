########## MACROS ###########################################################################
#############################################################################################

# Requires CMake > 3.15
if(${CMAKE_VERSION} VERSION_LESS "3.15")
    message(FATAL_ERROR "The 'CMakeDeps' generator only works with CMake >= 3.15")
endif()

if(mongoc-1.0_FIND_QUIETLY)
    set(mongoc-1.0_MESSAGE_MODE VERBOSE)
else()
    set(mongoc-1.0_MESSAGE_MODE STATUS)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/cmakedeps_macros.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/mongoc-1.0Targets.cmake)
include(CMakeFindDependencyMacro)

check_build_type_defined()

foreach(_DEPENDENCY ${mongo-c-driver_FIND_DEPENDENCY_NAMES} )
    # Check that we have not already called a find_package with the transitive dependency
    if(NOT ${_DEPENDENCY}_FOUND)
        find_dependency(${_DEPENDENCY} REQUIRED ${${_DEPENDENCY}_FIND_MODE})
    endif()
endforeach()

set(mongoc-1.0_VERSION_STRING "1.22.0")
set(mongoc-1.0_INCLUDE_DIRS ${mongo-c-driver_INCLUDE_DIRS_RELEASE} )
set(mongoc-1.0_INCLUDE_DIR ${mongo-c-driver_INCLUDE_DIRS_RELEASE} )
set(mongoc-1.0_LIBRARIES ${mongo-c-driver_LIBRARIES_RELEASE} )
set(mongoc-1.0_DEFINITIONS ${mongo-c-driver_DEFINITIONS_RELEASE} )

# Only the first installed configuration is included to avoid the collision
foreach(_BUILD_MODULE ${mongo-c-driver_BUILD_MODULES_PATHS_RELEASE} )
    message(${mongoc-1.0_MESSAGE_MODE} "Conan: Including build module from '${_BUILD_MODULE}'")
    include(${_BUILD_MODULE})
endforeach()


