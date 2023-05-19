########## MACROS ###########################################################################
#############################################################################################

# Requires CMake > 3.15
if(${CMAKE_VERSION} VERSION_LESS "3.15")
    message(FATAL_ERROR "The 'CMakeDeps' generator only works with CMake >= 3.15")
endif()

if(GTest_FIND_QUIETLY)
    set(GTest_MESSAGE_MODE VERBOSE)
else()
    set(GTest_MESSAGE_MODE STATUS)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/cmakedeps_macros.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/GTestTargets.cmake)
include(CMakeFindDependencyMacro)

check_build_type_defined()

foreach(_DEPENDENCY ${gtest_FIND_DEPENDENCY_NAMES} )
    # Check that we have not already called a find_package with the transitive dependency
    if(NOT ${_DEPENDENCY}_FOUND)
        find_dependency(${_DEPENDENCY} REQUIRED ${${_DEPENDENCY}_FIND_MODE})
    endif()
endforeach()

set(GTest_VERSION_STRING "1.12.1")
set(GTest_INCLUDE_DIRS ${gtest_INCLUDE_DIRS_RELEASE} )
set(GTest_INCLUDE_DIR ${gtest_INCLUDE_DIRS_RELEASE} )
set(GTest_LIBRARIES ${gtest_LIBRARIES_RELEASE} )
set(GTest_DEFINITIONS ${gtest_DEFINITIONS_RELEASE} )

# Only the first installed configuration is included to avoid the collision
foreach(_BUILD_MODULE ${gtest_BUILD_MODULES_PATHS_RELEASE} )
    message(${GTest_MESSAGE_MODE} "Conan: Including build module from '${_BUILD_MODULE}'")
    include(${_BUILD_MODULE})
endforeach()


