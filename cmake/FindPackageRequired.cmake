macro(find_package_required package debpackage)
    find_package(${package})
    if(NOT ${package}_FOUND)
        message(FATAL_ERROR
            "Cmake module for ${package} not found.\n"
            "Please install '${debpackage}' package.")
    endif()
endmacro()

macro(find_package_required_version package debpackage version)
    find_package(${package} ${version})
    if(NOT ${package}_FOUND)
        message(FATAL_ERROR
            "Cmake module for ${package} version ${version} not found.\n"
            "Please install '${debpackage}' package.")
    endif()
endmacro()
