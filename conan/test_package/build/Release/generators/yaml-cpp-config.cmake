########## MACROS ###########################################################################
#############################################################################################

# Requires CMake > 3.15
if(${CMAKE_VERSION} VERSION_LESS "3.15")
    message(FATAL_ERROR "The 'CMakeDeps' generator only works with CMake >= 3.15")
endif()

if(yaml-cpp_FIND_QUIETLY)
    set(yaml-cpp_MESSAGE_MODE VERBOSE)
else()
    set(yaml-cpp_MESSAGE_MODE STATUS)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/cmakedeps_macros.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/yaml-cppTargets.cmake)
include(CMakeFindDependencyMacro)

check_build_type_defined()

foreach(_DEPENDENCY ${yaml-cpp_FIND_DEPENDENCY_NAMES} )
    # Check that we have not already called a find_package with the transitive dependency
    if(NOT ${_DEPENDENCY}_FOUND)
        find_dependency(${_DEPENDENCY} REQUIRED ${${_DEPENDENCY}_FIND_MODE})
    endif()
endforeach()

set(yaml-cpp_VERSION_STRING "0.7.0")
set(yaml-cpp_INCLUDE_DIRS ${yaml-cpp_INCLUDE_DIRS_RELEASE} )
set(yaml-cpp_INCLUDE_DIR ${yaml-cpp_INCLUDE_DIRS_RELEASE} )
set(yaml-cpp_LIBRARIES ${yaml-cpp_LIBRARIES_RELEASE} )
set(yaml-cpp_DEFINITIONS ${yaml-cpp_DEFINITIONS_RELEASE} )

# Only the first installed configuration is included to avoid the collision
foreach(_BUILD_MODULE ${yaml-cpp_BUILD_MODULES_PATHS_RELEASE} )
    message(${yaml-cpp_MESSAGE_MODE} "Conan: Including build module from '${_BUILD_MODULE}'")
    include(${_BUILD_MODULE})
endforeach()


