include_guard(GLOBAL)

# Installs files, directories, or programs into the component install tree.
#
# This is the single approved install helper for non-target, non-config-file
# content.  All installs must go through this function (or _userver_install_targets
# for targets) so that the config-bypass guard below is uniformly enforced.
#
# @param COMPONENT      Component name (required).
# @param DESTINATION    Install destination path (required).
# @param FILES          List of files to install.
# @param DIRECTORY      Directory to install (recursive).
# @param PROGRAMS       List of programs (executables) to install.
# @param PATTERN        Files-matching pattern when installing a DIRECTORY.
# @param RENAME         Optional rename for a single installed file/program.
# @multiparam EXCLUDE_PATTERNS  Patterns to exclude when installing a DIRECTORY.
function(_userver_directory_install)
    if(NOT USERVER_INSTALL)
        return()
    endif()
    set(option)
    set(oneValueArgs COMPONENT DESTINATION PATTERN RENAME)
    set(multiValueArgs FILES DIRECTORY PROGRAMS EXCLUDE_PATTERNS)
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

    # Guard: userver-<comp>-config files must be installed only via
    # _userver_install_component_config(), not as raw FILES entries.  The
    # generator assembles the full config from the body fragment and the
    # link-graph; bypassing it would ship an incomplete or stale file.
    foreach(_guard_file IN LISTS ARG_FILES)
        if(_guard_file MATCHES "userver-[a-zA-Z0-9_-]+-config(-body)?\\.cmake(\\.in)?$")
            message(
                FATAL_ERROR
                    "_userver_directory_install: '${_guard_file}' must not be " "installed as a raw FILES entry.\n"
                    "Use _userver_install_component_config(<component>) instead; "
                    "see cmake/PrepareInstall.cmake for the config-install workflow."
            )
        endif()
    endforeach()

    if(ARG_PROGRAMS)
        install(
            PROGRAMS ${ARG_PROGRAMS}
            DESTINATION ${ARG_DESTINATION}
            COMPONENT ${ARG_COMPONENT}
            RENAME ${ARG_RENAME}
        )
    endif()
    if(ARG_FILES)
        install(
            FILES ${ARG_FILES}
            DESTINATION ${ARG_DESTINATION}
            COMPONENT ${ARG_COMPONENT}
            RENAME ${ARG_RENAME}
        )
    endif()
    if(ARG_DIRECTORY)
        set(install_args)
        if(ARG_PATTERN)
            list(APPEND install_args FILES_MATCHING PATTERN ${ARG_PATTERN})
        endif()
        foreach(pattern IN LISTS ARG_EXCLUDE_PATTERNS)
            list(APPEND install_args PATTERN "${pattern}" EXCLUDE)
        endforeach()
        install(
            DIRECTORY ${ARG_DIRECTORY}
            DESTINATION ${ARG_DESTINATION}
            COMPONENT ${ARG_COMPONENT}
            USE_SOURCE_PERMISSIONS ${install_args}
        )
    endif()
endfunction()
