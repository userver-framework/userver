include_guard(GLOBAL)

# Testsuite venv / requirements assembly.
#
# This file provides the functions that translate a set of active database
# names into pip requirements files and into a provisioned Python venv with
# the testsuite extras installed.  It depends on:
#   • userver_venv_setup   — defined in UserverVenv.cmake
#   • _userver_testsuite_databases_to_pip_info — defined in UserverTestsuiteDbRegistry.cmake
#   • _userver_testsuite_active_databases      — defined in UserverTestsuiteDbRegistry.cmake
# All of the above must be included before this file is included.

# Returns the base (non-DB) requirements files needed by any userver testsuite
# venv: the core requirements.txt plus, when gRPC is active, the matching
# requirements-grpc-<category>.txt.
#
# @param OUT_FILES  Variable to set to the list of requirements file paths.
function(_userver_testsuite_base_requirements OUT_FILES)
    get_property(userver_cmake_dir GLOBAL PROPERTY userver_cmake_dir)
    get_property(userver_testsuite_dir GLOBAL PROPERTY userver_testsuite_dir)

    set(req_files "${userver_testsuite_dir}/requirements.txt")

    if(USERVER_FEATURE_GRPC OR TARGET userver::grpc)
        get_property(protobuf_category GLOBAL PROPERTY userver_protobuf_version_category)
        if(NOT protobuf_category)
            include("${userver_cmake_dir}/SetupProtobuf.cmake")
            get_property(protobuf_category GLOBAL PROPERTY userver_protobuf_version_category)
        endif()
        list(APPEND req_files "${userver_testsuite_dir}/requirements-grpc-${protobuf_category}.txt")
    endif()

    set("${OUT_FILES}"
        "${req_files}"
        PARENT_SCOPE
    )
endfunction()

# Builds the requirements file list needed to set up a utest venv that can
# start the given databases via `python -m testsuite.environment --databases=...`.
# Writes a generated requirements-userver-testsuite-<key>.txt into CMAKE_BINARY_DIR.
#
# @param OUT_REQ_FILES  Variable to set to the resulting list of requirements files.
# @param OUT_KEY        Variable to set to a stable string key for the database set
#                       (empty string when databases list is empty).
# @option NON_STRICT    Unknown DB tokens emit a WARNING instead of the default FATAL_ERROR.
# @param ARGN           Database names.
function(_userver_testsuite_env_requirements OUT_REQ_FILES OUT_KEY)
    set(options NON_STRICT)
    cmake_parse_arguments(_ter "${options}" "" "" ${ARGN})

    get_property(USERVER_TESTSUITE_DIR GLOBAL PROPERTY userver_testsuite_dir)

    set(databases "${_ter_UNPARSED_ARGUMENTS}")
    list(SORT databases)
    list(REMOVE_DUPLICATES databases)

    # Build a stable key from the sorted database names (empty → use plain "utest").
    if(databases)
        list(JOIN databases "-" key)
    else()
        set(key "")
    endif()

    set(non_strict_flag "")
    if(_ter_NON_STRICT)
        set(non_strict_flag NON_STRICT)
    endif()

    _userver_testsuite_databases_to_pip_info(db_modules db_req_files ${non_strict_flag} ${databases})

    # Substitute the pip extras into the testsuite requirements line.
    file(READ "${USERVER_TESTSUITE_DIR}/requirements-testsuite.txt" testsuite_req_text)
    if(db_modules)
        list(JOIN db_modules "," db_modules_str)
        string(REPLACE "yandex-taxi-testsuite[]" "yandex-taxi-testsuite[${db_modules_str}]" testsuite_req_text
                       "${testsuite_req_text}"
        )
    endif()

    if(key)
        set(generated_file "${CMAKE_BINARY_DIR}/requirements-userver-testsuite-${key}.txt")
    else()
        set(generated_file "${CMAKE_BINARY_DIR}/requirements-userver-testsuite.txt")
    endif()
    file(WRITE "${generated_file}" "${testsuite_req_text}")

    set(req_files "${generated_file}" ${db_req_files})

    set("${OUT_REQ_FILES}"
        "${req_files}"
        PARENT_SCOPE
    )
    set("${OUT_KEY}"
        "${key}"
        PARENT_SCOPE
    )
endfunction()

# Creates (or reuses) a testsuite venv for a given set of database names and
# generates the matching env-script once.
#
# The venv is named "<VENV_PREFIX>" when DATABASES is empty, or
# "<VENV_PREFIX>-<db_key>" otherwise, where <db_key> is the sorted, dash-joined
# list of database names.  The venv is UNIQUE so tests with the same set share
# one venv.
#
# The env-script is written to
#   ${CMAKE_BINARY_DIR}/testsuite/env        (no databases)
#   ${CMAKE_BINARY_DIR}/testsuite/env-<key>  (with databases)
# and is generated at most once per <db_key> (guarded by a GLOBAL property).
#
# @param OUT_ENV_SCRIPT     Variable to receive the absolute env-script path.
# @param OUT_PYTHON_BINARY  Variable to receive the python binary from the venv.
# @param VENV_PREFIX        Name prefix for the venv (e.g. "utest", "userver-default").
# @option NON_STRICT        Unknown DB tokens → WARNING instead of the default FATAL_ERROR.
# @multiparam DATABASES     Database names (may be empty).
function(_userver_ensure_testsuite_venv_for_databases OUT_ENV_SCRIPT OUT_PYTHON_BINARY)
    set(options NON_STRICT)
    set(oneValueArgs VENV_PREFIX)
    set(multiValueArgs DATABASES)
    cmake_parse_arguments(_etvfd "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT _etvfd_VENV_PREFIX)
        message(FATAL_ERROR "_userver_ensure_testsuite_venv_for_databases: VENV_PREFIX is required")
    endif()

    get_property(USERVER_TESTSUITE_DIR GLOBAL PROPERTY userver_testsuite_dir)

    set(non_strict_flag "")
    if(_etvfd_NON_STRICT)
        set(non_strict_flag NON_STRICT)
    endif()

    _userver_testsuite_env_requirements(db_req_files db_key ${non_strict_flag} ${_etvfd_DATABASES})

    if(db_key)
        set(venv_name "${_etvfd_VENV_PREFIX}-${db_key}")
        set(env_script "${CMAKE_BINARY_DIR}/testsuite/env-${db_key}")
    else()
        set(venv_name "${_etvfd_VENV_PREFIX}")
        set(env_script "${CMAKE_BINARY_DIR}/testsuite/env")
    endif()

    userver_venv_setup(
        NAME "${venv_name}"
        # TESTSUITE_PYTHON_BINARY is used in env.in
        PYTHON_OUTPUT_VAR venv_python
        REQUIREMENTS ${db_req_files}
        UNIQUE
    )

    # Generate the env-script once per db_key (memoized by a GLOBAL property).
    get_property(_env_generated GLOBAL PROPERTY "userver_testsuite_env_generated_${db_key}")
    if(NOT _env_generated)
        set(TESTSUITE_PYTHON_BINARY "${venv_python}")
        configure_file("${USERVER_TESTSUITE_DIR}/env.in" "${env_script}" @ONLY)
        set_property(GLOBAL PROPERTY "userver_testsuite_env_generated_${db_key}" TRUE)
    endif()

    set("${OUT_ENV_SCRIPT}"
        "${env_script}"
        PARENT_SCOPE
    )
    set("${OUT_PYTHON_BINARY}"
        "${venv_python}"
        PARENT_SCOPE
    )
endfunction()

# Returns the requirements files needed for a userver-based service's testsuite.
# This is the "public" interface: service CMakeLists.txt call this to get the
# files to pass to userver_venv_setup / pip install.
#
# For private dependencies used only by userver's own tests, see
# testsuite/SetupUserverTestsuiteEnv.cmake.
#
# @option TESTSUITE_ONLY  When set, return only the generated testsuite
#                         requirements file (first element of env_req_files).
# @param REQUIREMENTS_FILES_VAR  (required) Variable name to store the result.
function(userver_testsuite_requirements)
    set(options TESTSUITE_ONLY)
    set(oneValueArgs REQUIREMENTS_FILES_VAR)
    set(multiValueArgs)

    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" "${ARGN}")

    _userver_testsuite_base_requirements(requirements_files)

    _userver_testsuite_active_databases(active_databases)

    # Auto-detected DB set (registry-derived, never user-supplied): NON_STRICT.
    _userver_testsuite_env_requirements(env_req_files env_key NON_STRICT ${active_databases})
    list(APPEND requirements_files ${env_req_files})

    if(NOT ARG_TESTSUITE_ONLY)
        set("${ARG_REQUIREMENTS_FILES_VAR}"
            ${requirements_files}
            PARENT_SCOPE
        )
    else()
        # TESTSUITE_ONLY: return only the generated testsuite requirements file.
        # It is always the first element of env_req_files (see _userver_testsuite_env_requirements).
        list(GET env_req_files 0 requirements_testsuite_file)
        set("${ARG_REQUIREMENTS_FILES_VAR}"
            ${requirements_testsuite_file}
            PARENT_SCOPE
        )
    endif()
endfunction()
