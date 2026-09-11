include_guard(GLOBAL)

# ─── Config-file install infrastructure ──────────────────────────────────────
#
# Every installed userver-<comp>-config.cmake is generated from a source
# template cmake/install/userver-<comp>-config.cmake.in via configure_file().
# The template carries a single placeholder:
#
#   @USERVER_CONFIG_USERVER_FIND_PACKAGE@
#
# which is substituted at install-configure time with a
# find_package(userver REQUIRED COMPONENTS ...) line derived from the live
# CMake link-graph.  This makes the declaration correct by construction:
# the generated line exactly matches what the component actually links, so
# "declared == needed" can never drift.
#
# Workflow
# --------
#   1. Each component's main CMakeLists.txt (or userver_module()) calls
#      _userver_install_component_config(<component>) to record the template
#      source path.  No direct install is performed at that point.
#
#   2. _userver_generate_and_install_configs() is called once at the top level
#      of the userver CMakeLists.txt, after all add_subdirectory() calls (so
#      the full link-graph is available).  For every recorded component it:
#        a. Computes cross-component userver deps from INTERFACE_LINK_LIBRARIES.
#        b. Renders USERVER_CONFIG_USERVER_FIND_PACKAGE from those deps.
#        c. configure_file()s the .cmake.in template → a .cmake in the build dir.
#        d. install(FILES <generated>)s it into the install tree.
#
#   3. _userver_check_component_config_deps() runs structured asserts:
#        - Every exported component with cross-component links must have a
#          recorded config template (catches missing recorder call / missing file).
#        - No generated file still contains the raw placeholder (catches a
#          template that omits @USERVER_CONFIG_USERVER_FIND_PACKAGE@).
#      Both checks operate on structured data — no text parsing of CMake source.
#
# Macro-vs-function contract
# --------------------------
# _userver_build_target_component_map and _userver_component_cross_deps are
# MACROS, not functions.  They write _target_comp_<tgt> variables into the
# CALLER'S scope to emulate a target→component hash-map.  The hash-map is
# created once by _userver_build_target_component_map() and read many times
# by _userver_component_cross_deps() within the same caller function.
# Do NOT convert these two macros to functions — doing so would break the
# shared-scope mechanism and silently emit empty COMPONENTS lists in every
# generated find_package() line.

# Populate _target_comp_<tgt> variables in the caller's scope.
# These variables emulate a hash-map: target name -> component name.
# They are only valid within the function that calls this macro.
macro(_userver_build_target_component_map)
    get_property(_btcm_exported GLOBAL PROPERTY USERVER_EXPORTED_COMPONENTS)
    foreach(_btcm_comp IN LISTS _btcm_exported)
        get_property(_btcm_targets GLOBAL PROPERTY USERVER_COMPONENT_TARGETS_${_btcm_comp})
        foreach(_btcm_tgt IN LISTS _btcm_targets)
            set(_target_comp_${_btcm_tgt} "${_btcm_comp}")
        endforeach()
    endforeach()
    unset(_btcm_exported)
    unset(_btcm_comp)
    unset(_btcm_targets)
    unset(_btcm_tgt)
endmacro()

# Walk INTERFACE_LINK_LIBRARIES of every target in <component> and set
# <out_var> to the deduplicated list of other component names they link.
# Requires _target_comp_* variables to be populated in the same scope (call
# _userver_build_target_component_map() first).
#
# Implemented as a macro so it shares the caller's variable scope and can read
# the _target_comp_* hash-map entries set by _userver_build_target_component_map.
macro(_userver_component_cross_deps _uccd_component _uccd_out_var)
    get_property(_uccd_comp_targets GLOBAL PROPERTY USERVER_COMPONENT_TARGETS_${_uccd_component})
    set(_uccd_cross_deps "")
    foreach(_uccd_tgt IN LISTS _uccd_comp_targets)
        if(NOT TARGET "${_uccd_tgt}")
            continue()
        endif()
        get_target_property(_uccd_links "${_uccd_tgt}" INTERFACE_LINK_LIBRARIES)
        if(NOT _uccd_links)
            continue()
        endif()
        foreach(_uccd_dep IN LISTS _uccd_links)
            # Normalize the namespaced alias form (userver::xxx) to the plain
            # target form (userver-xxx).  Chaotic-generated targets link their
            # dependencies via aliases (e.g. userver::chaotic), which the
            # filter below and the target->component map only recognise in
            # dash form.  Without this normalisation those deps are silently
            # skipped and the generated find_package() line omits the required
            # component, causing a "target not found" error downstream.
            string(REGEX REPLACE "^userver::" "userver-" _uccd_dep "${_uccd_dep}")
            # Only care about userver-* targets.
            if(NOT _uccd_dep MATCHES "^userver-")
                continue()
            endif()
            # Generator expressions — skip; they are not resolvable at
            # configure time.
            if(_uccd_dep MATCHES "\\$<")
                continue()
            endif()
            if(NOT DEFINED _target_comp_${_uccd_dep})
                continue()
            endif()
            set(_uccd_dep_comp "${_target_comp_${_uccd_dep}}")
            if(NOT _uccd_dep_comp STREQUAL _uccd_component)
                list(APPEND _uccd_cross_deps "${_uccd_dep_comp}")
            endif()
        endforeach()
    endforeach()
    list(REMOVE_DUPLICATES _uccd_cross_deps)
    set(${_uccd_out_var} "${_uccd_cross_deps}")
    unset(_uccd_comp_targets)
    unset(_uccd_tgt)
    unset(_uccd_links)
    unset(_uccd_dep)
    unset(_uccd_dep_comp)
    unset(_uccd_cross_deps)
    unset(_uccd_component)
    unset(_uccd_out_var)
endmacro()

# Record that <component>'s config file should be generated from the template
# cmake/install/userver-<component>-config.cmake.in.
#
# This is the ONLY sanctioned way to register a component config for install.
# Actual file generation and installation happen later in
# _userver_generate_and_install_configs(), called once at the top level after
# all add_subdirectory() calls so the full link-graph is available.
function(_userver_install_component_config component)
    if(NOT USERVER_INSTALL)
        return()
    endif()

    # NOTE: Do NOT use CMAKE_CURRENT_LIST_DIR here. Inside a function(),
    # CMAKE_CURRENT_LIST_DIR resolves to the CALLER's file directory (e.g.
    # universal/, core/, or a DB module dir via userver_module()), not to the
    # cmake/ directory where this function is defined.
    set(_body "${USERVER_ROOT_DIR}/cmake/install/userver-${component}-config-body.cmake.in")
    if(NOT EXISTS "${_body}")
        message(
            FATAL_ERROR
                "_userver_install_component_config: body fragment not found for "
                "component '${component}': ${_body}\n"
                "Create cmake/install/userver-${component}-config-body.cmake.in "
                "with the unique find_package / include() calls for this component "
                "(the boilerplate skeleton is generated automatically)."
        )
    endif()

    # Guard against duplicate registration.
    get_property(_already_registered GLOBAL PROPERTY USERVER_COMPONENT_CONFIG_SRC_${component})
    if(_already_registered)
        message(
            FATAL_ERROR
                "_userver_install_component_config: component '${component}' "
                "registered more than once.  Call _userver_install_component_config "
                "exactly once per main component."
        )
    endif()

    set_property(GLOBAL PROPERTY USERVER_COMPONENT_CONFIG_SRC_${component} "${_body}")
    set_property(GLOBAL APPEND PROPERTY USERVER_CONFIG_COMPONENTS "${component}")
endfunction()

# Generate userver-<comp>-config.cmake from the per-component body fragment
# and the shared boilerplate skeleton for every recorded component, then
# schedule the result for install.
#
# The skeleton wraps each body fragment with:
#   include_guard(GLOBAL)
#   [find_package(userver REQUIRED COMPONENTS ...) — from link-graph]
#   <body>
#   _userver_include_component_targets(<comp>)
#   set(userver_<comp>_FOUND TRUE)
#
# If the body fragment already contains _userver_include_component_targets
# (e.g. universal, which must load targets mid-body before calling
# userver_setup_environment()), the trailing call is omitted from the skeleton.
#
# Must be called once at the top level after all add_subdirectory() calls so
# that the link-graph is complete before cross-component deps are computed.
function(_userver_generate_and_install_configs)
    if(NOT USERVER_INSTALL)
        return()
    endif()

    get_property(_config_components GLOBAL PROPERTY USERVER_CONFIG_COMPONENTS)

    # Build the target->component map once; _userver_component_cross_deps
    # (a macro) will read _target_comp_* from this function's scope.
    _userver_build_target_component_map()

    set(_gen_dir "${CMAKE_CURRENT_BINARY_DIR}/generated-configs")
    file(MAKE_DIRECTORY "${_gen_dir}")

    foreach(_comp IN LISTS _config_components)
        get_property(_body_fragment GLOBAL PROPERTY USERVER_COMPONENT_CONFIG_SRC_${_comp})

        # Compute sorted cross-component deps from the link-graph.
        _userver_component_cross_deps("${_comp}" _cross_deps)
        list(SORT _cross_deps)

        # Build the find_package line (empty when there are no deps, e.g. universal).
        if(_cross_deps)
            string(JOIN " " _deps_str ${_cross_deps})
            set(_find_pkg "find_package(userver REQUIRED COMPONENTS ${_deps_str})\n\n")
        else()
            set(_find_pkg "")
        endif()

        # Read the component-specific body fragment.
        file(READ "${_body_fragment}" _body)

        # Detect whether the body already contains _userver_include_component_targets
        # (e.g. universal loads targets mid-body).  If so, skip the skeleton trailer.
        if(_body MATCHES "_userver_include_component_targets")
            set(_trailer "")
        else()
            set(_trailer "\n_userver_include_component_targets(${_comp})\n")
        endif()

        # Auto-inject the testsuite DB fragment include for components that ship
        # a UserverTestsuiteDb-<comp>.cmake fragment.  This mirrors the
        # userver_module() auto-include in UserverModule.cmake and removes the
        # need for a manual include() line in each *-config-body.cmake.in.
        set(_ts_fragment "${USERVER_ROOT_DIR}/cmake/testsuite/UserverTestsuiteDb-${_comp}.cmake")
        if(EXISTS "${_ts_fragment}")
            set(_ts_include "\ninclude(\"\${USERVER_TESTSUITE_DIR}/UserverTestsuiteDb-${_comp}.cmake\")\n")
        else()
            set(_ts_include "")
        endif()

        set(_content
            "include_guard(GLOBAL)\n\n${_find_pkg}${_body}${_ts_include}${_trailer}\nset(userver_${_comp}_FOUND TRUE)\n"
        )

        set(_generated "${_gen_dir}/userver-${_comp}-config.cmake")
        file(WRITE "${_generated}" "${_content}")

        install(
            FILES "${_generated}"
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/userver"
            COMPONENT "${_comp}"
        )
    endforeach()
endfunction()

# Structured defence check: verify that
#   1. Every exported component with cross-component links has a registered
#      body fragment (catches missing _userver_install_component_config call
#      or missing -body.cmake.in file).
#   2. Every generated config file does not contain leftover @ placeholders
#      (catches a body fragment with an unintended @ token that survived
#      into the assembled output).
#
# Both checks operate on structured data and file content of *generated* files
# only — no text parsing of CMake source.
#
# Call this explicitly at the top level after _userver_generate_and_install_configs()
# and alongside _userver_export_targets().
function(_userver_check_component_config_deps)
    if(NOT USERVER_INSTALL)
        return()
    endif()

    get_property(_exported_components GLOBAL PROPERTY USERVER_EXPORTED_COMPONENTS)

    _userver_build_target_component_map()

    set(_check_errors "")
    set(_gen_dir "${CMAKE_CURRENT_BINARY_DIR}/generated-configs")

    foreach(_comp IN LISTS _exported_components)
        _userver_component_cross_deps("${_comp}" _needed_deps)

        if(NOT _needed_deps)
            continue()
        endif()

        # Assert a body fragment was registered for this component.
        get_property(_body GLOBAL PROPERTY USERVER_COMPONENT_CONFIG_SRC_${_comp})
        if(NOT _body)
            string(
                APPEND
                _check_errors
                "  component '${_comp}' links userver targets from components "
                "(${_needed_deps}) but no config body fragment was registered.\n"
                "    Call _userver_install_component_config(${_comp}) from "
                "${_comp}/CMakeLists.txt (or via userver_module()) and create "
                "cmake/install/userver-${_comp}-config-body.cmake.in with the "
                "component-specific find_package / include() calls.\n"
            )
            continue()
        endif()

        # Assert the generated file contains no leftover @ placeholders.
        set(_generated "${_gen_dir}/userver-${_comp}-config.cmake")
        if(EXISTS "${_generated}")
            file(READ "${_generated}" _gen_contents)
            if(_gen_contents MATCHES "@[A-Z_]+@")
                string(APPEND _check_errors "  userver-${_comp}-config.cmake contains a leftover @ "
                       "placeholder after generation.  Check "
                       "cmake/install/userver-${_comp}-config-body.cmake.in for " "unintended @ tokens.\n"
                )
            endif()
        endif()
    endforeach()

    if(_check_errors)
        message(FATAL_ERROR "userver install config dependency check failed:\n${_check_errors}"
                            "See cmake/PrepareInstall.cmake for the config-install workflow."
        )
    endif()
endfunction()
