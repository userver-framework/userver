# Load the debug and release variables
get_filename_component(_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
file(GLOB DATA_FILES "${_DIR}/grpc-proto-*-data.cmake")

foreach(f ${DATA_FILES})
    include(${f})
endforeach()

# Create the targets for all the components
foreach(_COMPONENT ${grpc-proto_COMPONENT_NAMES} )
    if(NOT TARGET ${_COMPONENT})
        add_library(${_COMPONENT} INTERFACE IMPORTED)
        message(${grpc-proto_MESSAGE_MODE} "Conan: Component target declared '${_COMPONENT}'")
    endif()
endforeach()

if(NOT TARGET grpc-proto::grpc-proto)
    add_library(grpc-proto::grpc-proto INTERFACE IMPORTED)
    message(${grpc-proto_MESSAGE_MODE} "Conan: Target declared 'grpc-proto::grpc-proto'")
endif()
# Load the debug and release library finders
get_filename_component(_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
file(GLOB CONFIG_FILES "${_DIR}/grpc-proto-Target-*.cmake")

foreach(f ${CONFIG_FILES})
    include(${f})
endforeach()