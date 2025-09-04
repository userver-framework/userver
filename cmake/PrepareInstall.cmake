include_guard(GLOBAL)

set_property(GLOBAL PROPERTY userver_cmake_dir "${CMAKE_CURRENT_LIST_DIR}")

function(_userver_install_targets)
    set(oneValueArgs COMPONENT)
    set(multiValueArgs TARGETS)
    cmake_parse_arguments(ARG "${option}" "${oneValueArgs}" "${multiValueArgs}" "${ARGN}")
    if(NOT ARG_COMPONENT)
        message(FATAL_ERROR "No COMPONENT for install")
    endif()
    if(NOT ARG_TARGETS)
        message(FATAL_ERROR "No TARGETS given for install")
    endif()

    foreach(target IN LISTS ARG_TARGETS)
        if(NOT TARGET ${target})
            message(FATAL_ERROR "${target} is not a target. You should use only targets")
        endif()
        if(target MATCHES "^userver-.*")
            string(REGEX REPLACE "^userver-" "" target_without_userver "${target}")
            add_library("userver::${target_without_userver}" ALIAS "${target}")
            set_target_properties("${target}" PROPERTIES EXPORT_NAME "${target_without_userver}")
        endif()
    endforeach()

    if(NOT USERVER_INSTALL)
        return()
    endif()

    install(
        TARGETS ${ARG_TARGETS}
        EXPORT userver-targets
        CONFIGURATIONS RELEASE
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${ARG_COMPONENT}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${ARG_COMPONENT}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${ARG_COMPONENT}
        INCLUDES
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
    install(
        TARGETS ${ARG_TARGETS}
        EXPORT userver-targets_d
        CONFIGURATIONS DEBUG
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${ARG_COMPONENT}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${ARG_COMPONENT}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${ARG_COMPONENT}
        INCLUDES
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
endfunction()

function(_userver_export_targets)
    if(NOT USERVER_INSTALL)
        return()
    endif()
    install(
        EXPORT userver-targets
        FILE userver-targets.cmake
        CONFIGURATIONS RELEASE
        NAMESPACE userver::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/userver/release
        COMPONENT universal
    )
    install(
        EXPORT userver-targets_d
        FILE userver-targets_d.cmake
        CONFIGURATIONS DEBUG
        NAMESPACE userver::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/userver/debug
        COMPONENT universal
    )
endfunction()

function(_userver_directory_install)
    if(NOT USERVER_INSTALL)
        return()
    endif()
    set(option)
    set(oneValueArgs COMPONENT DESTINATION PATTERN)
    set(multiValueArgs FILES DIRECTORY PROGRAMS)
    cmake_parse_arguments(ARG "${option}" "${oneValueArgs}" "${multiValueArgs}" "${ARGN}")
    if(NOT ARG_COMPONENT)
        message(FATAL_ERROR "No COMPONENT for install")
    endif()
    if(NOT ARG_DESTINATION)
        message(FATAL_ERROR "No DESTINATION for install")
    endif()
    if(NOT ARG_FILES
       AND NOT ARG_DIRECTORY
       AND NOT ARG_PROGRAMS
    )
        message(FATAL_ERROR "No FILES or DIRECTORY or PROGRAMS provided to install")
    endif()
    if(ARG_PROGRAMS)
        install(
            PROGRAMS ${ARG_PROGRAMS}
            DESTINATION ${ARG_DESTINATION}
            COMPONENT ${ARG_COMPONENT}
        )
    endif()
    if(ARG_FILES)
        install(
            FILES ${ARG_FILES}
            DESTINATION ${ARG_DESTINATION}
            COMPONENT ${ARG_COMPONENT}
        )
    endif()
    if(ARG_DIRECTORY)
        if(ARG_PATTERN)
            install(
                DIRECTORY ${ARG_DIRECTORY}
                DESTINATION ${ARG_DESTINATION}
                COMPONENT ${ARG_COMPONENT}
                USE_SOURCE_PERMISSIONS FILES_MATCHING
                PATTERN ${ARG_PATTERN}
            )
        else()
            install(
                DIRECTORY ${ARG_DIRECTORY}
                DESTINATION ${ARG_DESTINATION}
                COMPONENT ${ARG_COMPONENT}
                USE_SOURCE_PERMISSIONS
            )
        endif()
    endif()
endfunction()

function(_userver_make_install_config)
    if(NOT USERVER_INSTALL)
        return()
    endif()

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

function(_userver_install_component)
    if(NOT USERVER_INSTALL)
        return()
    endif()

    set(oneValueArgs MODULE)
    set(multiValueArgs DEPENDS)
    cmake_parse_arguments(ARG "${option}" "${oneValueArgs}" "${multiValueArgs}" "${ARGN}")

    string(TOUPPER "${ARG_MODULE}" MODULE_UPPER)
    if(CPACK_COMPONENTS_GROUPING STREQUAL ONE_PER_GROUP)
        if(NOT CPACK_DEBIAN_${MODULE_UPPER}_PACKAGE_DEPENDS)
	    message(FATAL_ERROR "File with per-component dependencies is missing (component ${ARG_MODULE}). Either use CPACK_COMPONENTS_GROUPING=ALL_COMPONENTS_IN_ONE to build a single all-in-one package, or create dependency file ${USERVER_ROOT_DIR}/scripts/docs/en/deps/${DEPENDENCIES_FILESTEM}/${ARG_MODULE}.")
        endif()
    endif()

    execute_process(
        COMMAND cat "${USERVER_ROOT_DIR}/scripts/docs/en/deps/${DEPENDENCIES_FILESTEM}/${ARG_MODULE}"
        COMMAND tr "\n" " "
        COMMAND sed "s/ \\(.\\)/, \\1/g"
        OUTPUT_VARIABLE MODULE_DEPENDS
    )
    file(APPEND "${CMAKE_BINARY_DIR}/cpack.variables.inc" "
        set(CPACK_DEBIAN_${MODULE_UPPER}_PACKAGE_NAME libuserver-${ARG_MODULE}-dev)
        set(CPACK_DEBIAN_${MODULE_UPPER}_PACKAGE_CONFLICTS libuserver-all-dev)
        set(CPACK_COMPONENT_${MODULE_UPPER}_DEPENDS ${ARG_DEPENDS})
        set(CPACK_DEBIAN_${MODULE_UPPER}_PACKAGE_DEPENDS \"${MODULE_DEPENDS}\")
    ")

    file(APPEND "${CMAKE_BINARY_DIR}/cpack.inc" "
        cpack_add_component_group(${ARG_MODULE} EXPANDED)
        cpack_add_component(${ARG_MODULE} GROUP ${ARG_MODULE} INSTALL_TYPES Full)
    ")
endfunction()

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
    elseif(OS_CODENAME MATCHES "^jammy")
        set(DEPENDENCIES_FILESTEM "ubuntu-22.04")
    elseif(OS_CODENAME MATCHES "^impish")
        set(DEPENDENCIES_FILESTEM "ubuntu-21.04")
    elseif(OS_CODENAME MATCHES "^focal")
        set(DEPENDENCIES_FILESTEM "ubuntu-20.04")
    elseif(OS_CODENAME MATCHES "^bionic")
        set(DEPENDENCIES_FILESTEM "ubuntu-18.04")
    endif()
    set(DEPENDENCIES_FILESTEM ${DEPENDENCIES_FILESTEM} CACHE INTERNAL "")
endfunction()

_userver_prepare_components()
