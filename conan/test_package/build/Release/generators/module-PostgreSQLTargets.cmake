# Load the debug and release variables
get_filename_component(_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
file(GLOB DATA_FILES "${_DIR}/module-PostgreSQL-*-data.cmake")

foreach(f ${DATA_FILES})
    include(${f})
endforeach()

# Create the targets for all the components
foreach(_COMPONENT ${libpq_COMPONENT_NAMES} )
    if(NOT TARGET ${_COMPONENT})
        add_library(${_COMPONENT} INTERFACE IMPORTED)
        message(${PostgreSQL_MESSAGE_MODE} "Conan: Component target declared '${_COMPONENT}'")
    endif()
endforeach()

if(NOT TARGET PostgreSQL::PostgreSQL)
    add_library(PostgreSQL::PostgreSQL INTERFACE IMPORTED)
    message(${PostgreSQL_MESSAGE_MODE} "Conan: Target declared 'PostgreSQL::PostgreSQL'")
endif()
# Load the debug and release library finders
get_filename_component(_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
file(GLOB CONFIG_FILES "${_DIR}/module-PostgreSQL-Target-*.cmake")

foreach(f ${CONFIG_FILES})
    include(${f})
endforeach()