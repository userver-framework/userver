########## MACROS ###########################################################################
#############################################################################################

# Requires CMake > 3.15
if(${CMAKE_VERSION} VERSION_LESS "3.15")
    message(FATAL_ERROR "The 'CMakeDeps' generator only works with CMake >= 3.15")
endif()

if(amqpcpp_FIND_QUIETLY)
    set(amqpcpp_MESSAGE_MODE VERBOSE)
else()
    set(amqpcpp_MESSAGE_MODE STATUS)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/cmakedeps_macros.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/amqpcppTargets.cmake)
include(CMakeFindDependencyMacro)

check_build_type_defined()

foreach(_DEPENDENCY ${amqp-cpp_FIND_DEPENDENCY_NAMES} )
    # Check that we have not already called a find_package with the transitive dependency
    if(NOT ${_DEPENDENCY}_FOUND)
        find_dependency(${_DEPENDENCY} REQUIRED ${${_DEPENDENCY}_FIND_MODE})
    endif()
endforeach()

set(amqpcpp_VERSION_STRING "4.3.16")
set(amqpcpp_INCLUDE_DIRS ${amqp-cpp_INCLUDE_DIRS_RELEASE} )
set(amqpcpp_INCLUDE_DIR ${amqp-cpp_INCLUDE_DIRS_RELEASE} )
set(amqpcpp_LIBRARIES ${amqp-cpp_LIBRARIES_RELEASE} )
set(amqpcpp_DEFINITIONS ${amqp-cpp_DEFINITIONS_RELEASE} )

# Only the first installed configuration is included to avoid the collision
foreach(_BUILD_MODULE ${amqp-cpp_BUILD_MODULES_PATHS_RELEASE} )
    message(${amqpcpp_MESSAGE_MODE} "Conan: Including build module from '${_BUILD_MODULE}'")
    include(${_BUILD_MODULE})
endforeach()


