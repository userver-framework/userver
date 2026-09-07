include_guard(GLOBAL)

# DB Registry — dependency-injection mechanism for testsuite database modules.
#
# This file owns the registry MECHANISM; each
# cmake/testsuite/UserverTestsuiteDb-<mod>.cmake fragment owns its DATA and
# calls userver_testsuite_register_database from there.
#
# CONTRACT: userver_testsuite_register_database MUST NOT be called from
# UserverTestsuite.cmake itself; only from per-module fragments.  That
# separation means this file can be included independently (e.g. from an
# installed find_package config) without pulling in any DB-specific logic.
#
# Per-module fragments are auto-included in two ways:
#   • In-tree build: userver_module() in cmake/UserverModule.cmake includes
#     cmake/testsuite/UserverTestsuiteDb-<mod>.cmake when it exists.
#   • deb-install flow: the generated userver-<mod>-config.cmake includes
#     the installed testsuite/UserverTestsuiteDb-<mod>.cmake fragment
#     (injected by _userver_generate_and_install_configs in InstallComponentConfigGen.cmake).

# Registers a testsuite database so the testsuite venv infra knows how to start
# it and so userver_testsuite_requirements can auto-detect it from loaded modules.
#
# @multiparam NAMES  One or more --databases= / DATABASES tokens owned by this
#                    module (e.g. "redis" "redis-cluster", or "postgresql" "postgres").
#                    The first token is the canonical name; the rest are aliases.
# @param PIP_MODULE         testsuite pip extra module name (e.g. "mongodb", "redis").
#                           Omit (or leave empty) when the DB has no pip extra (e.g. ydb).
# @param PIP_MODULE_DARWIN  Optional Darwin-specific override for PIP_MODULE
#                           (e.g. "postgresql-binary").
# @param REQUIREMENTS_FILE  Optional absolute path to a per-DB pip requirements file
#                           (e.g. testsuite/requirements-mongo.txt). May reference
#                           ${USERVER_TESTSUITE_DIR} via a relative path with no leading
#                           slash; the reader will resolve it.
# @param FEATURE_VAR        Optional USERVER_FEATURE_* CMake variable name whose truth
#                           value indicates this module is active
#                           (e.g. USERVER_FEATURE_MONGODB).
#                           Used by userver_testsuite_requirements to auto-detect active DBs.
# @param TARGET             Optional CMake target name whose existence indicates this
#                           module is active (e.g. "userver::mongo").
#                           Used by userver_testsuite_requirements to auto-detect active DBs.
function(userver_testsuite_register_database)
    set(options)
    set(oneValueArgs PIP_MODULE PIP_MODULE_DARWIN REQUIREMENTS_FILE FEATURE_VAR TARGET)
    set(multiValueArgs NAMES)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT ARG_NAMES)
        message(FATAL_ERROR "userver_testsuite_register_database: NAMES is required")
    endif()

    # The canonical (first) name is what we append to the global list; aliases
    # are stored under their own keys but point at the same pip info.
    list(GET ARG_NAMES 0 canonical_name)

    # Append canonical name to the global registry list (deduplication: only if
    # not already registered under this canonical name).
    get_property(registered GLOBAL PROPERTY userver_testsuite_registered_databases)
    list(FIND registered "${canonical_name}" already_idx)
    if(already_idx EQUAL -1)
        set_property(GLOBAL APPEND PROPERTY userver_testsuite_registered_databases "${canonical_name}")
    endif()

    # Store pip info under every name in NAMES (canonical + aliases).
    foreach(db_name IN LISTS ARG_NAMES)
        set_property(GLOBAL PROPERTY "userver_testsuite_db_${db_name}_pip_module" "${ARG_PIP_MODULE}")
        set_property(GLOBAL PROPERTY "userver_testsuite_db_${db_name}_pip_module_darwin" "${ARG_PIP_MODULE_DARWIN}")
        set_property(GLOBAL PROPERTY "userver_testsuite_db_${db_name}_req_file" "${ARG_REQUIREMENTS_FILE}")
    endforeach()

    # Store activation hints only under the canonical name.
    set_property(GLOBAL PROPERTY "userver_testsuite_db_${canonical_name}_feature_var" "${ARG_FEATURE_VAR}")
    set_property(GLOBAL PROPERTY "userver_testsuite_db_${canonical_name}_target" "${ARG_TARGET}")

    # Ship the calling fragment file.
    if(USERVER_INSTALL)
        _userver_directory_install(
            COMPONENT "${canonical_name}"
            FILES "${CMAKE_CURRENT_LIST_FILE}"
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/userver/testsuite"
        )
    endif()
endfunction()

# Reads the registry populated by userver_testsuite_register_database and
# resolves a single database token to its pip module name and requirements file.
#
# @param database      Database token (e.g. "mongo", "redis-cluster").
# @param OUT_MODULE    Variable to receive the pip extra module name, or "".
# @param OUT_REQ_FILE  Variable to receive the requirements file path, or "".
# @option NON_STRICT   When set, an unregistered token emits a WARNING instead
#                      of the default FATAL_ERROR.  Pass this only for
#                      auto-detection scans where the token list is
#                      registry-derived and an unknown entry is benign.
function(_userver_testsuite_database_to_pip_info database OUT_MODULE OUT_REQ_FILE)
    set(options NON_STRICT)
    cmake_parse_arguments(_dtpi "${options}" "" "" ${ARGN})

    get_property(pip_module GLOBAL PROPERTY "userver_testsuite_db_${database}_pip_module")
    get_property(pip_module_darwin GLOBAL PROPERTY "userver_testsuite_db_${database}_pip_module_darwin")
    get_property(req_file GLOBAL PROPERTY "userver_testsuite_db_${database}_req_file")

    # Check if the token was ever registered (all three properties are unset for
    # an unknown token; an empty pip_module is valid, e.g. for ydb).
    get_property(
        module_set GLOBAL
        PROPERTY "userver_testsuite_db_${database}_pip_module"
        SET
    )
    if(NOT module_set)
        if(_dtpi_NON_STRICT)
            message(WARNING "_userver_testsuite_database_to_pip_info: unknown database '${database}'. "
                            "Did you forget to include the corresponding UserverTestsuiteDb-<mod>.cmake fragment?"
            )
        else()
            message(
                FATAL_ERROR
                    "_userver_testsuite_database_to_pip_info: unknown database '${database}'. "
                    "Did you forget to include the corresponding "
                    "UserverTestsuiteDb-<mod>.cmake fragment (in-tree) or the matching "
                    "userver-<mod> component (deb-install)?"
            )
        endif()
        set("${OUT_MODULE}"
            ""
            PARENT_SCOPE
        )
        set("${OUT_REQ_FILE}"
            ""
            PARENT_SCOPE
        )
        return()
    endif()

    # Apply Darwin override when present.
    if(CMAKE_SYSTEM_NAME MATCHES "Darwin" AND pip_module_darwin)
        set(pip_module "${pip_module_darwin}")
    endif()

    set("${OUT_MODULE}"
        "${pip_module}"
        PARENT_SCOPE
    )
    set("${OUT_REQ_FILE}"
        "${req_file}"
        PARENT_SCOPE
    )
endfunction()

# Computes the testsuite pip extra modules and per-DB requirements files for a
# given list of database names. Deduplicates modules so that e.g. "redis" and
# "redis-cluster" together contribute only one "redis" module entry.
#
# @param OUT_MODULES    Variable to set to a list of pip extra module names.
# @param OUT_REQ_FILES  Variable to set to a list of per-DB requirements files.
# @option NON_STRICT    Unknown tokens emit a WARNING instead of the default FATAL_ERROR.
# @param ARGN           Database names (same values as DATABASES in userver_add_utest).
function(_userver_testsuite_databases_to_pip_info OUT_MODULES OUT_REQ_FILES)
    set(options NON_STRICT)
    cmake_parse_arguments(_dtpi2 "${options}" "" "" ${ARGN})

    set(non_strict_flag "")
    if(_dtpi2_NON_STRICT)
        set(non_strict_flag NON_STRICT)
    endif()

    set(modules "")
    set(req_files "")
    foreach(database IN LISTS _dtpi2_UNPARSED_ARGUMENTS)
        _userver_testsuite_database_to_pip_info("${database}" db_module db_req_file ${non_strict_flag})
        if(db_module)
            list(APPEND modules "${db_module}")
        endif()
        if(db_req_file)
            list(APPEND req_files "${db_req_file}")
        endif()
    endforeach()
    list(REMOVE_DUPLICATES modules)
    list(REMOVE_DUPLICATES req_files)
    set("${OUT_MODULES}"
        "${modules}"
        PARENT_SCOPE
    )
    set("${OUT_REQ_FILES}"
        "${req_files}"
        PARENT_SCOPE
    )
endfunction()

# Derives the set of registered testsuite DB modules that a given CMake target
# links against (directly or transitively).
#
# @param target   CMake target whose link graph is inspected.
# @param OUT_DBS  Variable to set to the sorted, deduplicated canonical DB name list.
function(_userver_testsuite_active_databases_for_target target OUT_DBS)
    set(result "")

    if(NOT TARGET "${target}")
        set("${OUT_DBS}"
            ""
            PARENT_SCOPE
        )
        return()
    endif()

    _userver_collect_linked_targets("${target}" linked_targets)

    get_property(registered_dbs GLOBAL PROPERTY userver_testsuite_registered_databases)
    foreach(db_canonical IN LISTS registered_dbs)
        get_property(db_target GLOBAL PROPERTY "userver_testsuite_db_${db_canonical}_target")
        if(NOT db_target)
            continue()
        endif()

        # Accept both the alias form (userver::mongo) and the resolved dash form
        # (userver-mongo) since _userver_collect_linked_targets resolves aliases.
        string(REPLACE "::" "-" db_target_dash "${db_target}")
        if(db_target IN_LIST linked_targets OR db_target_dash IN_LIST linked_targets)
            list(APPEND result "${db_canonical}")
        endif()
    endforeach()

    list(SORT result)
    list(REMOVE_DUPLICATES result)
    set("${OUT_DBS}"
        "${result}"
        PARENT_SCOPE
    )
endfunction()

# Unified "which DBs are active?" helper.
#
# @param OUT_DBS   Variable to set to the sorted, deduplicated canonical DB list.
# @param TARGET    (optional) CMake target whose link graph to inspect.
function(_userver_testsuite_active_databases OUT_DBS)
    cmake_parse_arguments(ARG "" "TARGET" "" ${ARGN})

    if(ARG_TARGET AND TARGET "${ARG_TARGET}")
        _userver_testsuite_active_databases_for_target("${ARG_TARGET}" result)
    else()
        # Registry scan: check FEATURE_VAR / TARGET hints for every registered DB.
        set(result "")
        get_property(registered_dbs GLOBAL PROPERTY userver_testsuite_registered_databases)
        foreach(db_canonical IN LISTS registered_dbs)
            get_property(feature_var GLOBAL PROPERTY "userver_testsuite_db_${db_canonical}_feature_var")
            get_property(db_target GLOBAL PROPERTY "userver_testsuite_db_${db_canonical}_target")
            set(db_active FALSE)
            if(feature_var AND ${feature_var})
                set(db_active TRUE)
            endif()
            if(db_target AND TARGET "${db_target}")
                set(db_active TRUE)
            endif()
            if(db_active)
                list(APPEND result "${db_canonical}")
            endif()
        endforeach()
    endif()

    set("${OUT_DBS}"
        "${result}"
        PARENT_SCOPE
    )
endfunction()
