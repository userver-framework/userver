########## MACROS ###########################################################################
#############################################################################################

# Requires CMake > 3.15
if(${CMAKE_VERSION} VERSION_LESS "3.15")
    message(FATAL_ERROR "The 'CMakeDeps' generator only works with CMake >= 3.15")
endif()

if(CURL_FIND_QUIETLY)
    set(CURL_MESSAGE_MODE VERBOSE)
else()
    set(CURL_MESSAGE_MODE STATUS)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/cmakedeps_macros.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/CURLTargets.cmake)
include(CMakeFindDependencyMacro)

check_build_type_defined()

foreach(_DEPENDENCY ${libcurl_FIND_DEPENDENCY_NAMES} )
    # Check that we have not already called a find_package with the transitive dependency
    if(NOT ${_DEPENDENCY}_FOUND)
        find_dependency(${_DEPENDENCY} REQUIRED ${${_DEPENDENCY}_FIND_MODE})
    endif()
endforeach()

set(CURL_VERSION_STRING "7.86.0")
set(CURL_INCLUDE_DIRS ${libcurl_INCLUDE_DIRS_RELEASE} )
set(CURL_INCLUDE_DIR ${libcurl_INCLUDE_DIRS_RELEASE} )
set(CURL_LIBRARIES ${libcurl_LIBRARIES_RELEASE} )
set(CURL_DEFINITIONS ${libcurl_DEFINITIONS_RELEASE} )

# Only the first installed configuration is included to avoid the collision
foreach(_BUILD_MODULE ${libcurl_BUILD_MODULES_PATHS_RELEASE} )
    message(${CURL_MESSAGE_MODE} "Conan: Including build module from '${_BUILD_MODULE}'")
    include(${_BUILD_MODULE})
endforeach()


