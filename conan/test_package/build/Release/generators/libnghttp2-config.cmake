########## MACROS ###########################################################################
#############################################################################################

# Requires CMake > 3.15
if(${CMAKE_VERSION} VERSION_LESS "3.15")
    message(FATAL_ERROR "The 'CMakeDeps' generator only works with CMake >= 3.15")
endif()

if(libnghttp2_FIND_QUIETLY)
    set(libnghttp2_MESSAGE_MODE VERBOSE)
else()
    set(libnghttp2_MESSAGE_MODE STATUS)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/cmakedeps_macros.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/libnghttp2Targets.cmake)
include(CMakeFindDependencyMacro)

check_build_type_defined()

foreach(_DEPENDENCY ${libnghttp2_FIND_DEPENDENCY_NAMES} )
    # Check that we have not already called a find_package with the transitive dependency
    if(NOT ${_DEPENDENCY}_FOUND)
        find_dependency(${_DEPENDENCY} REQUIRED ${${_DEPENDENCY}_FIND_MODE})
    endif()
endforeach()

set(libnghttp2_VERSION_STRING "1.51.0")
set(libnghttp2_INCLUDE_DIRS ${libnghttp2_INCLUDE_DIRS_RELEASE} )
set(libnghttp2_INCLUDE_DIR ${libnghttp2_INCLUDE_DIRS_RELEASE} )
set(libnghttp2_LIBRARIES ${libnghttp2_LIBRARIES_RELEASE} )
set(libnghttp2_DEFINITIONS ${libnghttp2_DEFINITIONS_RELEASE} )

# Only the first installed configuration is included to avoid the collision
foreach(_BUILD_MODULE ${libnghttp2_BUILD_MODULES_PATHS_RELEASE} )
    message(${libnghttp2_MESSAGE_MODE} "Conan: Including build module from '${_BUILD_MODULE}'")
    include(${_BUILD_MODULE})
endforeach()


