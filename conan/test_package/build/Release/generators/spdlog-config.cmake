########## MACROS ###########################################################################
#############################################################################################

# Requires CMake > 3.15
if(${CMAKE_VERSION} VERSION_LESS "3.15")
    message(FATAL_ERROR "The 'CMakeDeps' generator only works with CMake >= 3.15")
endif()

if(spdlog_FIND_QUIETLY)
    set(spdlog_MESSAGE_MODE VERBOSE)
else()
    set(spdlog_MESSAGE_MODE STATUS)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/cmakedeps_macros.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/spdlogTargets.cmake)
include(CMakeFindDependencyMacro)

check_build_type_defined()

foreach(_DEPENDENCY ${spdlog_FIND_DEPENDENCY_NAMES} )
    # Check that we have not already called a find_package with the transitive dependency
    if(NOT ${_DEPENDENCY}_FOUND)
        find_dependency(${_DEPENDENCY} REQUIRED ${${_DEPENDENCY}_FIND_MODE})
    endif()
endforeach()

set(spdlog_VERSION_STRING "1.9.0")
set(spdlog_INCLUDE_DIRS ${spdlog_INCLUDE_DIRS_RELEASE} )
set(spdlog_INCLUDE_DIR ${spdlog_INCLUDE_DIRS_RELEASE} )
set(spdlog_LIBRARIES ${spdlog_LIBRARIES_RELEASE} )
set(spdlog_DEFINITIONS ${spdlog_DEFINITIONS_RELEASE} )

# Only the first installed configuration is included to avoid the collision
foreach(_BUILD_MODULE ${spdlog_BUILD_MODULES_PATHS_RELEASE} )
    message(${spdlog_MESSAGE_MODE} "Conan: Including build module from '${_BUILD_MODULE}'")
    include(${_BUILD_MODULE})
endforeach()


