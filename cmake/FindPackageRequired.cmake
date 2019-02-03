macro(init_debian_depends)
    set(DEBIAN_DEPENDS "" CACHE INTERNAL DEBIAN_DEPENDS)
endmacro()

macro(append_debian_depends PACKAGE)
    string(REGEX REPLACE " +" ";" PACKAGE_LIST ${PACKAGE})
    list(APPEND DEBIAN_DEPENDS ${PACKAGE_LIST})
    set(DEBIAN_DEPENDS "${DEBIAN_DEPENDS}" CACHE INTERNAL DEBIAN_DEPENDS)
endmacro()

macro(find_package_required PACKAGE debpackage)
    find_package(${PACKAGE})
    append_debian_depends(${debpackage})
    string(TOUPPER "${PACKAGE}" PACKAGE_UPPERCASE)

    if(NOT ${PACKAGE_UPPERCASE}_FOUND AND NOT ${PACKAGE}_FOUND)
        message(FATAL_ERROR
                "Cmake module for ${PACKAGE} not found.\n"
                "Please install '${debpackage}' package.")
    endif()
endmacro()

macro(find_package_components_required PACKAGE debpackage)
    find_package(${PACKAGE} COMPONENTS ${ARGN})
    append_debian_depends(${debpackage})
    string(TOUPPER "${PACKAGE}" PACKAGE_UPPERCASE)

    if(NOT ${PACKAGE_UPPERCASE}_FOUND AND NOT ${PACKAGE}_FOUND)
        message(FATAL_ERROR
                "Cmake module for ${PACKAGE} not found.\n"
                "Please install '${debpackage}' package.")
    endif()
endmacro()

macro(find_package_required_version PACKAGE debpackage version)
    find_package(${PACKAGE} ${version})
    append_debian_depends(${debpackage})

    string(TOUPPER "${PACKAGE}" PACKAGE_UPPERCASE)
    if(NOT ${PACKAGE_UPPERCASE}_FOUND AND NOT ${PACKAGE}_FOUND)
        message(FATAL_ERROR
                "Cmake module for ${PACKAGE} version ${version} not found.\n"
                "Please install '${debpackage}' package.")
    endif()
endmacro()

macro(find_program_required PROGRAM debpackage)
    find_program(BINARY ${PROGRAM})
    append_debian_depends(${debpackage})

    if(${BINARY} STREQUAL BINARY-NOTFOUND)
        message(FATAL_ERROR
                "Program ${PROGRAM} not found in $PATH.\n"
                "Please install '${debpackage}' package.")
    endif()
endmacro()
