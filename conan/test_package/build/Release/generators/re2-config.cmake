########## MACROS ###########################################################################
#############################################################################################

# Requires CMake > 3.15
if(${CMAKE_VERSION} VERSION_LESS "3.15")
    message(FATAL_ERROR "The 'CMakeDeps' generator only works with CMake >= 3.15")
endif()

if(re2_FIND_QUIETLY)
    set(re2_MESSAGE_MODE VERBOSE)
else()
    set(re2_MESSAGE_MODE STATUS)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/cmakedeps_macros.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/re2Targets.cmake)
include(CMakeFindDependencyMacro)

check_build_type_defined()

foreach(_DEPENDENCY ${re2_FIND_DEPENDENCY_NAMES} )
    # Check that we have not already called a find_package with the transitive dependency
    if(NOT ${_DEPENDENCY}_FOUND)
        find_dependency(${_DEPENDENCY} REQUIRED ${${_DEPENDENCY}_FIND_MODE})
    endif()
endforeach()

set(re2_VERSION_STRING "20220601")
set(re2_INCLUDE_DIRS ${re2_INCLUDE_DIRS_RELEASE} )
set(re2_INCLUDE_DIR ${re2_INCLUDE_DIRS_RELEASE} )
set(re2_LIBRARIES ${re2_LIBRARIES_RELEASE} )
set(re2_DEFINITIONS ${re2_DEFINITIONS_RELEASE} )

# Only the first installed configuration is included to avoid the collision
foreach(_BUILD_MODULE ${re2_BUILD_MODULES_PATHS_RELEASE} )
    message(${re2_MESSAGE_MODE} "Conan: Including build module from '${_BUILD_MODULE}'")
    include(${_BUILD_MODULE})
endforeach()


