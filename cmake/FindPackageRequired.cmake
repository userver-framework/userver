macro(init_debian_depends)
    set(DEBIAN_DEPENDS "" CACHE INTERNAL DEBIAN_DEPENDS)
endmacro()

macro(append_debian_depends PACKAGE)
    set(DEBIAN_DEPENDS "${DEBIAN_DEPENDS}, ${PACKAGE}" CACHE INTERNAL DEBIAN_DEPENDS)
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
