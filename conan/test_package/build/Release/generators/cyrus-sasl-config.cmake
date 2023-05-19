########## MACROS ###########################################################################
#############################################################################################

# Requires CMake > 3.15
if(${CMAKE_VERSION} VERSION_LESS "3.15")
    message(FATAL_ERROR "The 'CMakeDeps' generator only works with CMake >= 3.15")
endif()

if(cyrus-sasl_FIND_QUIETLY)
    set(cyrus-sasl_MESSAGE_MODE VERBOSE)
else()
    set(cyrus-sasl_MESSAGE_MODE STATUS)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/cmakedeps_macros.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cyrus-saslTargets.cmake)
include(CMakeFindDependencyMacro)

check_build_type_defined()

foreach(_DEPENDENCY ${cyrus-sasl_FIND_DEPENDENCY_NAMES} )
    # Check that we have not already called a find_package with the transitive dependency
    if(NOT ${_DEPENDENCY}_FOUND)
        find_dependency(${_DEPENDENCY} REQUIRED ${${_DEPENDENCY}_FIND_MODE})
    endif()
endforeach()

set(cyrus-sasl_VERSION_STRING "2.1.27")
set(cyrus-sasl_INCLUDE_DIRS ${cyrus-sasl_INCLUDE_DIRS_RELEASE} )
set(cyrus-sasl_INCLUDE_DIR ${cyrus-sasl_INCLUDE_DIRS_RELEASE} )
set(cyrus-sasl_LIBRARIES ${cyrus-sasl_LIBRARIES_RELEASE} )
set(cyrus-sasl_DEFINITIONS ${cyrus-sasl_DEFINITIONS_RELEASE} )

# Only the first installed configuration is included to avoid the collision
foreach(_BUILD_MODULE ${cyrus-sasl_BUILD_MODULES_PATHS_RELEASE} )
    message(${cyrus-sasl_MESSAGE_MODE} "Conan: Including build module from '${_BUILD_MODULE}'")
    include(${_BUILD_MODULE})
endforeach()


