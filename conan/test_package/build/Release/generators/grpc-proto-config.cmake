########## MACROS ###########################################################################
#############################################################################################

# Requires CMake > 3.15
if(${CMAKE_VERSION} VERSION_LESS "3.15")
    message(FATAL_ERROR "The 'CMakeDeps' generator only works with CMake >= 3.15")
endif()

if(grpc-proto_FIND_QUIETLY)
    set(grpc-proto_MESSAGE_MODE VERBOSE)
else()
    set(grpc-proto_MESSAGE_MODE STATUS)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/cmakedeps_macros.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/grpc-protoTargets.cmake)
include(CMakeFindDependencyMacro)

check_build_type_defined()

foreach(_DEPENDENCY ${grpc-proto_FIND_DEPENDENCY_NAMES} )
    # Check that we have not already called a find_package with the transitive dependency
    if(NOT ${_DEPENDENCY}_FOUND)
        find_dependency(${_DEPENDENCY} REQUIRED ${${_DEPENDENCY}_FIND_MODE})
    endif()
endforeach()

set(grpc-proto_VERSION_STRING "cci.20220627")
set(grpc-proto_INCLUDE_DIRS ${grpc-proto_INCLUDE_DIRS_RELEASE} )
set(grpc-proto_INCLUDE_DIR ${grpc-proto_INCLUDE_DIRS_RELEASE} )
set(grpc-proto_LIBRARIES ${grpc-proto_LIBRARIES_RELEASE} )
set(grpc-proto_DEFINITIONS ${grpc-proto_DEFINITIONS_RELEASE} )

# Only the first installed configuration is included to avoid the collision
foreach(_BUILD_MODULE ${grpc-proto_BUILD_MODULES_PATHS_RELEASE} )
    message(${grpc-proto_MESSAGE_MODE} "Conan: Including build module from '${_BUILD_MODULE}'")
    include(${_BUILD_MODULE})
endforeach()


