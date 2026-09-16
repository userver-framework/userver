include_guard(GLOBAL)

# Generates the top-level userverConfig.cmake and userverConfigVersion.cmake
# and schedules them for install into lib/cmake/userver/.
#
# USERVER_AVAILABLE_COMPONENTS is collected automatically from prior
# _userver_install_component() calls and is substituted into Config.cmake.in
# as the default list of find_package(userver) components.
function(_userver_make_install_config)
    if(NOT USERVER_INSTALL)
        return()
    endif()

    get_property(USERVER_AVAILABLE_COMPONENTS GLOBAL PROPERTY USERVER_AVAILABLE_COMPONENTS)

    configure_package_config_file(
        "${USERVER_ROOT_DIR}/cmake/install/Config.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}/userverConfig.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/userver"
    )

    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/userverConfigVersion.cmake"
        VERSION "${USERVER_VERSION}"
        COMPATIBILITY SameMajorVersion
    )

    _userver_directory_install(
        COMPONENT universal
        FILES "${CMAKE_CURRENT_BINARY_DIR}/userverConfig.cmake" "${CMAKE_CURRENT_BINARY_DIR}/userverConfigVersion.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/userver"
    )
endfunction()

# Registers a component for CPack / Debian packaging and appends its metadata
# to the cpack.inc and cpack.variables.inc include-fragments in the build dir.
#
# @param COMPONENT   Component name (required).
# @option NON_FINDABLE  When set, the component is NOT added to the
#                       USERVER_AVAILABLE_COMPONENTS list (i.e. it is not
#                       directly find_package()-able as a standalone component).
# @multiparam DEPENDS  CMake-level component dependencies (CPACK_COMPONENT_*_DEPENDS).
function(_userver_install_component)
    if(NOT USERVER_INSTALL)
        return()
    endif()

    set(options NON_FINDABLE)
    set(oneValueArgs COMPONENT)
    set(multiValueArgs DEPENDS)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" "${ARGN}")

    if(NOT ARG_NON_FINDABLE)
        set_property(GLOBAL APPEND PROPERTY USERVER_AVAILABLE_COMPONENTS ${ARG_COMPONENT})
    endif()

    string(TOUPPER "${ARG_COMPONENT}" COMPONENT_UPPER)
    if(CPACK_COMPONENTS_GROUPING STREQUAL ONE_PER_GROUP)
        if(NOT CPACK_DEBIAN_${COMPONENT_UPPER}_PACKAGE_DEPENDS)
            message(
                FATAL_ERROR
                    "File with per-component dependencies is missing (component ${ARG_COMPONENT}). "
                    "Either use CPACK_COMPONENTS_GROUPING=ALL_COMPONENTS_IN_ONE to build a single "
                    "all-in-one package, or create dependency file "
                    "${USERVER_ROOT_DIR}/scripts/docs/en/deps/${DEPENDENCIES_FILESTEM}/${ARG_COMPONENT}."
            )
        endif()
    endif()

    # Read the per-component Debian dependency file and convert the
    # newline-separated list to the ", "-separated string required by
    # CPACK_DEBIAN_*_PACKAGE_DEPENDS.
    #
    # The deps file format: one package name per line, trailing newline.
    # Required output format: "pkg1, pkg2, ..., pkgN " (comma-space-separated,
    # trailing space from the last newline — matches the historic cat|tr|sed pipeline).
    #
    # When no deps file exists for a component (e.g. chaotic-openapi) the
    # historic cat|tr|sed pipeline silently produced an empty string; we
    # preserve that behaviour here.
    set(_deps_file "${USERVER_ROOT_DIR}/scripts/docs/en/deps/${DEPENDENCIES_FILESTEM}/${ARG_COMPONENT}")
    if(EXISTS "${_deps_file}")
        file(READ "${_deps_file}" COMPONENT_DEPENDS)
        # Trailing newline → space (no comma before EOF, matching sed behaviour).
        string(REGEX REPLACE "\n$" " " COMPONENT_DEPENDS "${COMPONENT_DEPENDS}")
        # Internal newlines → ", ".
        string(REGEX REPLACE "\n" ", " COMPONENT_DEPENDS "${COMPONENT_DEPENDS}")
    else()
        set(COMPONENT_DEPENDS "")
    endif()

    file(
        APPEND "${CMAKE_BINARY_DIR}/cpack.variables.inc"
        "
        set(CPACK_DEBIAN_${COMPONENT_UPPER}_PACKAGE_NAME libuserver-${ARG_COMPONENT}-dev)
        set(CPACK_DEBIAN_${COMPONENT_UPPER}_PACKAGE_CONFLICTS libuserver-all-dev)
        set(CPACK_COMPONENT_${COMPONENT_UPPER}_DEPENDS ${ARG_DEPENDS})
        set(CPACK_DEBIAN_${COMPONENT_UPPER}_PACKAGE_DEPENDS \"${COMPONENT_DEPENDS}\")
    "
    )

    file(
        APPEND "${CMAKE_BINARY_DIR}/cpack.inc"
        "
        cpack_add_component_group(${ARG_COMPONENT} EXPANDED)
        cpack_add_component(${ARG_COMPONENT} GROUP ${ARG_COMPONENT} INSTALL_TYPES Full)
    "
    )
endfunction()

# Detects the current OS/distro codename, sets DEPENDENCIES_FILESTEM to the
# matching deps subdirectory name, and clears any stale cpack*.inc fragments
# from a previous configure run.
#
# Must be called at include time (tail of PrepareInstall.cmake) so that
# DEPENDENCIES_FILESTEM is available before any _userver_install_component()
# calls process component dependencies.
function(_userver_prepare_components)
    file(REMOVE "${CMAKE_BINARY_DIR}/cpack.inc")
    file(REMOVE "${CMAKE_BINARY_DIR}/cpack.variables.inc")

    # DEB dependencies:
    execute_process(COMMAND lsb_release -cs OUTPUT_VARIABLE OS_CODENAME)
    if(OS_CODENAME MATCHES "^bookworm")
        set(DEPENDENCIES_FILESTEM "debian-12")
    elseif(OS_CODENAME MATCHES "^bullseye")
        set(DEPENDENCIES_FILESTEM "debian-11")
    elseif(OS_CODENAME MATCHES "^noble")
        set(DEPENDENCIES_FILESTEM "ubuntu-24.04")
    elseif(OS_CODENAME MATCHES "^resolute")
        set(DEPENDENCIES_FILESTEM "ubuntu-26.04")
    elseif(OS_CODENAME MATCHES "^jammy")
        set(DEPENDENCIES_FILESTEM "ubuntu-22.04")
    elseif(OS_CODENAME MATCHES "^impish")
        set(DEPENDENCIES_FILESTEM "ubuntu-21.04")
    elseif(OS_CODENAME MATCHES "^focal")
        set(DEPENDENCIES_FILESTEM "ubuntu-20.04")
    elseif(OS_CODENAME MATCHES "^bionic")
        set(DEPENDENCIES_FILESTEM "ubuntu-18.04")
    endif()
    set(DEPENDENCIES_FILESTEM
        ${DEPENDENCIES_FILESTEM}
        CACHE INTERNAL ""
    )
endfunction()
