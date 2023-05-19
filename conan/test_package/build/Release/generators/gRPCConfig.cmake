########## MACROS ###########################################################################
#############################################################################################

# Requires CMake > 3.15
if(${CMAKE_VERSION} VERSION_LESS "3.15")
    message(FATAL_ERROR "The 'CMakeDeps' generator only works with CMake >= 3.15")
endif()

if(gRPC_FIND_QUIETLY)
    set(gRPC_MESSAGE_MODE VERBOSE)
else()
    set(gRPC_MESSAGE_MODE STATUS)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/cmakedeps_macros.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/gRPCTargets.cmake)
include(CMakeFindDependencyMacro)

check_build_type_defined()

foreach(_DEPENDENCY ${grpc_FIND_DEPENDENCY_NAMES} )
    # Check that we have not already called a find_package with the transitive dependency
    if(NOT ${_DEPENDENCY}_FOUND)
        find_dependency(${_DEPENDENCY} REQUIRED ${${_DEPENDENCY}_FIND_MODE})
    endif()
endforeach()

set(gRPC_VERSION_STRING "1.48.0")
set(gRPC_INCLUDE_DIRS ${grpc_INCLUDE_DIRS_RELEASE} )
set(gRPC_INCLUDE_DIR ${grpc_INCLUDE_DIRS_RELEASE} )
set(gRPC_LIBRARIES ${grpc_LIBRARIES_RELEASE} )
set(gRPC_DEFINITIONS ${grpc_DEFINITIONS_RELEASE} )

# Only the first installed configuration is included to avoid the collision
foreach(_BUILD_MODULE ${grpc_BUILD_MODULES_PATHS_RELEASE} )
    message(${gRPC_MESSAGE_MODE} "Conan: Including build module from '${_BUILD_MODULE}'")
    include(${_BUILD_MODULE})
endforeach()


