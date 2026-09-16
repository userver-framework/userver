include_guard(GLOBAL)

# Installs userver targets, registers userver:: aliases, and tracks them for
# per-component export-set generation.
#
# @param COMPONENT   Component name (required).
# @multiparam TARGETS  Targets to install (required).
function(_userver_install_targets)
    set(options)
    set(oneValueArgs COMPONENT)
    set(multiValueArgs TARGETS)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" "${ARGN}")
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

    # Register the component for per-component export (dedup).
    set_property(GLOBAL APPEND PROPERTY USERVER_EXPORTED_COMPONENTS "${ARG_COMPONENT}")
    get_property(_all_exported GLOBAL PROPERTY USERVER_EXPORTED_COMPONENTS)
    list(REMOVE_DUPLICATES _all_exported)
    set_property(GLOBAL PROPERTY USERVER_EXPORTED_COMPONENTS "${_all_exported}")

    # Track which targets belong to this component (for the defence check).
    set_property(GLOBAL APPEND PROPERTY USERVER_COMPONENT_TARGETS_${ARG_COMPONENT} ${ARG_TARGETS})

    # Single export set per component (no CONFIGURATIONS filter).
    install(
        TARGETS ${ARG_TARGETS}
        EXPORT userver-targets-${ARG_COMPONENT}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${ARG_COMPONENT}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${ARG_COMPONENT}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${ARG_COMPONENT}
        INCLUDES
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
endfunction()
