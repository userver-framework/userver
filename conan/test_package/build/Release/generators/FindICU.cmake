########## MACROS ###########################################################################
#############################################################################################

# Requires CMake > 3.15
if(${CMAKE_VERSION} VERSION_LESS "3.15")
    message(FATAL_ERROR "The 'CMakeDeps' generator only works with CMake >= 3.15")
endif()

if(ICU_FIND_QUIETLY)
    set(ICU_MESSAGE_MODE VERBOSE)
else()
    set(ICU_MESSAGE_MODE STATUS)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/cmakedeps_macros.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/module-ICUTargets.cmake)
include(CMakeFindDependencyMacro)

check_build_type_defined()

foreach(_DEPENDENCY ${icu_FIND_DEPENDENCY_NAMES} )
    # Check that we have not already called a find_package with the transitive dependency
    if(NOT ${_DEPENDENCY}_FOUND)
        find_dependency(${_DEPENDENCY} REQUIRED ${${_DEPENDENCY}_FIND_MODE})
    endif()
endforeach()

set(ICU_VERSION_STRING "72.1")
set(ICU_INCLUDE_DIRS ${icu_INCLUDE_DIRS_RELEASE} )
set(ICU_INCLUDE_DIR ${icu_INCLUDE_DIRS_RELEASE} )
set(ICU_LIBRARIES ${icu_LIBRARIES_RELEASE} )
set(ICU_DEFINITIONS ${icu_DEFINITIONS_RELEASE} )

# Only the first installed configuration is included to avoid the collision
foreach(_BUILD_MODULE ${icu_BUILD_MODULES_PATHS_RELEASE} )
    message(${ICU_MESSAGE_MODE} "Conan: Including build module from '${_BUILD_MODULE}'")
    include(${_BUILD_MODULE})
endforeach()


include(FindPackageHandleStandardArgs)
set(ICU_FOUND 1)
set(ICU_VERSION "72.1")

find_package_handle_standard_args(ICU
                                  REQUIRED_VARS ICU_VERSION
                                  VERSION_VAR ICU_VERSION)
mark_as_advanced(ICU_FOUND ICU_VERSION)
