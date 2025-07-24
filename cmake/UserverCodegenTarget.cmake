# Functions for registering files in userver-codegen target.
#
# This target generates all add_custom_command files that are registered in it, without actually compiling any C++ code.
#
# This allows to quickly generate sources for IDE indexing and other tooling. If a generator is compiled from sources,
# e.g. Protobuf, then it will still have to be built.
#
# userver-codegen is similar to CMake's 3.31+ "codegen" target, except that userver-codegen actually works.

include_guard(GLOBAL)

function(_userver_initialize_codegen_flag)
    set(CODEGEN)
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.31")
        cmake_policy(SET CMP0171 NEW)
        list(APPEND CODEGEN "CODEGEN")
    endif()
    set(CODEGEN
        ${CODEGEN}
        PARENT_SCOPE
    )
endfunction()

function(_userver_codegen_make_main_target)
    get_property(userver_codegen_targets GLOBAL PROPERTY userver_codegen_targets)
    add_custom_target(
        userver-codegen
        COMMENT "Completing userver codegen"
        VERBATIM
    )
    add_dependencies(userver-codegen ${userver_codegen_targets})
endfunction()

function(_userver_codegen_next_target_index RESULT_VAR)
    get_property(userver_codegen_target_index GLOBAL PROPERTY userver_codegen_target_index)
    if(NOT userver_codegen_target_index)
        set(userver_codegen_target_index 0)
    endif()

    math(EXPR userver_codegen_target_index "${userver_codegen_target_index} + 1")
    set_property(GLOBAL PROPERTY userver_codegen_target_index "${userver_codegen_target_index}")

    set("${RESULT_VAR}"
        "${userver_codegen_target_index}"
        PARENT_SCOPE
    )
endfunction()

function(_userver_codegen_make_directory_target)
    get_property(
        userver_codegen_files
        DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        PROPERTY userver_codegen_files
    )
    if(NOT userver_codegen_files)
        return()
    endif()

    _userver_codegen_next_target_index(index)

    add_custom_target(
        "userver-codegen-impl-${index}"
        DEPENDS ${userver_codegen_files}
        COMMENT "Completing userver codegen for ${CMAKE_CURRENT_SOURCE_DIR}"
        VERBATIM
    )

    set_property(GLOBAL APPEND PROPERTY userver_codegen_targets "userver-codegen-impl-${index}")
endfunction()

function(_userver_codegen_register_files FILES_LIST)
    if(CMAKE_VERSION VERSION_LESS "3.19" OR NOT FILES_LIST)
        return()
    endif()

    get_property(
        has_userver_codegen_files
        DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        PROPERTY userver_codegen_files
        SET
    )
    if(NOT has_userver_codegen_files)
        # On first codegen files in the current directory, schedule indexed codegen target creation.
        cmake_language(DEFER DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" CALL _userver_codegen_make_directory_target)

        get_property(
            has_userver_codegen_targets GLOBAL
            PROPERTY userver_codegen_targets
            SET
        )
        if(NOT has_userver_codegen_targets)
            # On first codegen invocation, schedule userver-codegen target creation.
            cmake_language(
                DEFER
                DIRECTORY
                "${CMAKE_SOURCE_DIR}"
                ID
                userver_codegen_make_main_target
                CALL
                _userver_codegen_make_main_target
            )
        endif()
    endif()

    set_property(
        DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        APPEND
        PROPERTY userver_codegen_files ${FILES_LIST}
    )
endfunction()
