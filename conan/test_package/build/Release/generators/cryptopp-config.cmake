########## MACROS ###########################################################################
#############################################################################################

# Requires CMake > 3.15
if(${CMAKE_VERSION} VERSION_LESS "3.15")
    message(FATAL_ERROR "The 'CMakeDeps' generator only works with CMake >= 3.15")
endif()

if(cryptopp_FIND_QUIETLY)
    set(cryptopp_MESSAGE_MODE VERBOSE)
else()
    set(cryptopp_MESSAGE_MODE STATUS)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/cmakedeps_macros.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cryptoppTargets.cmake)
include(CMakeFindDependencyMacro)

check_build_type_defined()

foreach(_DEPENDENCY ${cryptopp_FIND_DEPENDENCY_NAMES} )
    # Check that we have not already called a find_package with the transitive dependency
    if(NOT ${_DEPENDENCY}_FOUND)
        find_dependency(${_DEPENDENCY} REQUIRED ${${_DEPENDENCY}_FIND_MODE})
    endif()
endforeach()

set(cryptopp_VERSION_STRING "8.7.0")
set(cryptopp_INCLUDE_DIRS ${cryptopp_INCLUDE_DIRS_RELEASE} )
set(cryptopp_INCLUDE_DIR ${cryptopp_INCLUDE_DIRS_RELEASE} )
set(cryptopp_LIBRARIES ${cryptopp_LIBRARIES_RELEASE} )
set(cryptopp_DEFINITIONS ${cryptopp_DEFINITIONS_RELEASE} )

# Only the first installed configuration is included to avoid the collision
foreach(_BUILD_MODULE ${cryptopp_BUILD_MODULES_PATHS_RELEASE} )
    message(${cryptopp_MESSAGE_MODE} "Conan: Including build module from '${_BUILD_MODULE}'")
    include(${_BUILD_MODULE})
endforeach()


