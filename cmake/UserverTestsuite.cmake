# Functions for running testsuite tests.

# Provides:
#
# * USERVER_FEATURE_TESTSUITE option
# * userver_testsuite_requirements function that returns a list of requirements files needed to run userver testsuite
# * userver_testsuite_add function that registers a directory with testsuite tests in ctest. Note that userver testsuite
#   requires some arguments, they should be passed manually using PYTEST_ARGS
# * userver_testsuite_add_simple that automatically detects and fills in some PYTEST_ARGS
#
# Implementation note: public functions here should be usable even without a direct include of this script, so the
# functions should not rely on non-cache variables being present.
include_guard(GLOBAL)

# Pull in DB registry (userver_testsuite_register_database + query helpers).
include("${CMAKE_CURRENT_LIST_DIR}/UserverTestsuiteDbRegistry.cmake")

# Pull in venv / requirements assembly (_userver_testsuite_*_requirements,
# _userver_ensure_testsuite_venv_for_databases, userver_testsuite_requirements).
include("${CMAKE_CURRENT_LIST_DIR}/UserverTestsuiteVenv.cmake")

# Pack initialization into a function to avoid non-cache variable leakage.
function(_userver_prepare_testsuite)
    include("${CMAKE_CURRENT_LIST_DIR}/UserverVenv.cmake")
    include("${CMAKE_CURRENT_LIST_DIR}/TargetIteration.cmake")
    set_property(GLOBAL PROPERTY userver_cmake_dir "${CMAKE_CURRENT_LIST_DIR}")

    # @ingroup libraries
    option(USERVER_FEATURE_TESTSUITE "Enable functional tests via testsuite" ON)

    if(USERVER_FEATURE_TESTSUITE AND NOT USERVER_PYTHON_DEV_CHECKED)
        # find package python3-dev required by venv
        execute_process(COMMAND sh "-c" "command -v python3-config" OUTPUT_VARIABLE PYTHONCONFIG_FOUND)
        if(NOT PYTHONCONFIG_FOUND)
            message(FATAL_ERROR "Python dev is not found")
        endif()
        set(USERVER_PYTHON_DEV_CHECKED
            TRUE
            CACHE INTERNAL "" FORCE
        )
    endif()

    if(NOT USERVER_TESTSUITE_DIR)
        get_filename_component(USERVER_TESTSUITE_DIR "${CMAKE_CURRENT_LIST_DIR}/../testsuite" ABSOLUTE)
    endif()
    set_property(GLOBAL PROPERTY userver_testsuite_dir "${USERVER_TESTSUITE_DIR}")

    # Eagerly register all in-tree DB fragments so the DB registry is fully
    # populated before the shared testsuite venv is built (userver_testsuite_requirements
    # in testsuite/SetupUserverTestsuiteEnv.cmake runs before the DB add_subdirectory
    # calls that would otherwise register them lazily via userver_module()).
    # Each fragment has include_guard(GLOBAL), so the later userver_module() include
    # is a no-op. Guarded by NOT USERVER_INSTALL because registration has an install
    # side effect (ships the fragment for its component); install builds disable the
    # internal testsuite and must keep packaging unchanged.
    if(USERVER_FEATURE_TESTSUITE AND NOT USERVER_INSTALL)
        file(GLOB _userver_ts_db_fragments "${CMAKE_CURRENT_LIST_DIR}/testsuite/UserverTestsuiteDb-*.cmake")
        foreach(_userver_ts_db_fragment IN LISTS _userver_ts_db_fragments)
            include("${_userver_ts_db_fragment}")
        endforeach()
    endif()

    if(USERVER_FEATURE_TESTSUITE)
        # Create the base utest venv (no DB extras) for tests with no DATABASES.
        # Tests that declare DATABASES get a separate per-database-set venv provisioned
        # lazily in userver_add_utest, where all component targets already exist.
        _userver_ensure_testsuite_venv_for_databases(
            _userver_base_env_script TESTSUITE_PYTHON_BINARY VENV_PREFIX utest
            # No DATABASES: base venv only.
        )
    endif()
endfunction()

# Returns the first path from PATHS that exists on the filesystem.
# Sets OUT_VAR to that path, or leaves it unset if none exists.
# Used by userver_testsuite_add_simple to probe for optional config files.
#
# @param OUT_VAR  Variable to set to the first existing path.
# @param ARGN     Candidate paths to probe in order.
function(_userver_find_first_existing OUT_VAR)
    foreach(_ffe_path IN LISTS ARGN)
        if(EXISTS "${_ffe_path}")
            set("${OUT_VAR}"
                "${_ffe_path}"
                PARENT_SCOPE
            )
            return()
        endif()
    endforeach()
endfunction()

# TODO
function(userver_testsuite_add)
    set(oneValueArgs SERVICE_TARGET TEST_SUFFIX WORKING_DIRECTORY PYTHON_BINARY PRETTY_LOGS SQL_LIBRARY)
    set(multiValueArgs PYTEST_ARGS REQUIREMENTS PYTHONPATH TEST_ENV RESOURCE_LOCKS)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    _userver_setup_environment_validate_impl()

    include(CTest)

    get_property(USERVER_TESTSUITE_DIR GLOBAL PROPERTY userver_testsuite_dir)

    if(NOT ARG_SERVICE_TARGET)
        message(FATAL_ERROR "No SERVICE_TARGET given for testsuite")
        return()
    endif()

    if(NOT ARG_WORKING_DIRECTORY)
        set(ARG_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    endif()

    if(NOT DEFINED ARG_PRETTY_LOGS)
        set(ARG_PRETTY_LOGS ON)
    endif()

    if(NOT ARG_TEST_SUFFIX)
        set(service_target_with_suffix "${ARG_SERVICE_TARGET}")
    else()
        set(service_target_with_suffix "${ARG_SERVICE_TARGET}-${ARG_TEST_SUFFIX}")
    endif()
    set(testsuite_test_name "testsuite-${service_target_with_suffix}")

    if(NOT USERVER_FEATURE_TESTSUITE)
        message(STATUS "Testsuite test ${testsuite_test_name} is disabled")
        return()
    endif()

    if(ARG_PYTHON_BINARY)
        if(ARG_REQUIREMENTS)
            message(FATAL_ERROR "PYTHON_BINARY and REQUIREMENTS options are incompatible")
        endif()
        set(python_binary "${ARG_PYTHON_BINARY}")
    elseif(ARG_REQUIREMENTS)
        userver_testsuite_requirements(REQUIREMENTS_FILES_VAR requirements_files)
        list(APPEND requirements_files ${ARG_REQUIREMENTS})
        userver_venv_setup(
            NAME "${testsuite_test_name}"
            REQUIREMENTS ${requirements_files}
            PYTHON_OUTPUT_VAR python_binary
        )
    else()
        _userver_testsuite_active_databases(active_dbs TARGET "${ARG_SERVICE_TARGET}")
        _userver_testsuite_base_requirements(base_req_files)
        # Auto-detected DB set (link-graph, never user-supplied): NON_STRICT.
        _userver_testsuite_env_requirements(db_req_files db_key NON_STRICT ${active_dbs})

        set(requirements_files ${base_req_files} ${db_req_files})

        if(db_key)
            set(venv_name "userver-default-${db_key}")
        else()
            set(venv_name "userver-default")
        endif()

        userver_venv_setup(
            NAME "${venv_name}"
            REQUIREMENTS ${requirements_files}
            PYTHON_OUTPUT_VAR python_binary
            UNIQUE
        )
    endif()

    if(NOT python_binary)
        message(FATAL_ERROR "No python binary given.")
    endif()

    set(TESTSUITE_RUNNER "${CMAKE_CURRENT_BINARY_DIR}/runtests-${service_target_with_suffix}")
    list(APPEND ARG_PYTHONPATH "${USERVER_TESTSUITE_DIR}/pytest_plugins")

    set(testsuite_temp_dir "${CMAKE_CURRENT_BINARY_DIR}/Testing/Temporary/${service_target_with_suffix}")
    file(MAKE_DIRECTORY "${testsuite_temp_dir}")

    set(TESTS_PATHS ${ARG_WORKING_DIRECTORY})
    if(ARG_SQL_LIBRARY)
        get_target_property(TESTSUITE_OUTPUT_DIR ${ARG_SQL_LIBRARY} USERVER_TESTSUITE_DIRECTORY)
        list(APPEND ARG_PYTEST_ARGS "-p" "sql_files")
        list(APPEND ARG_PYTEST_ARGS "-p" "pytest_userver.plugins.sql_coverage")
        list(APPEND ARG_PYTHONPATH ${TESTSUITE_OUTPUT_DIR})
    endif()

    _userver_initialize_codegen_flag()
    add_custom_command(
        OUTPUT "${TESTSUITE_RUNNER}"
        COMMAND
            "${python_binary}" "${USERVER_TESTSUITE_DIR}/create_runner.py" "--output=${TESTSUITE_RUNNER}"
            "--python=${python_binary}" "--tests-path=${TESTS_PATHS}" "--working-dir=${CMAKE_CURRENT_BINARY_DIR}"
            "--python-path=${ARG_PYTHONPATH}" -- "--build-dir=${CMAKE_CURRENT_BINARY_DIR}"
            "--service-logs-file=${testsuite_temp_dir}/service.log" "--basetemp=${testsuite_temp_dir}"
            ${ARG_PYTEST_ARGS}
        DEPENDS "${USERVER_TESTSUITE_DIR}/create_runner.py"
        COMMENT "Creating testsuite runner at ${TESTSUITE_RUNNER}"
        VERBATIM ${CODEGEN}
    )
    _userver_codegen_register_files("${TESTSUITE_RUNNER}")

    set(CREATE_TESTSUITE_RUNNER_TARGET "create-runtests-${service_target_with_suffix}")
    add_custom_target("${CREATE_TESTSUITE_RUNNER_TARGET}" SOURCES "${TESTSUITE_RUNNER}")
    add_dependencies("${ARG_SERVICE_TARGET}" "${CREATE_TESTSUITE_RUNNER_TARGET}")

    set(PRETTY_LOGS_MODE "")
    if(ARG_PRETTY_LOGS)
        set(PRETTY_LOGS_MODE "--service-logs-pretty")
    endif()

    # Pre-collect command in a list to support spaces in paths
    set(testsuite_test_command)
    list(APPEND testsuite_test_command "${python_binary}")
    list(APPEND testsuite_test_command "${TESTSUITE_RUNNER}")
    list(APPEND testsuite_test_command ${PRETTY_LOGS_MODE})
    list(APPEND testsuite_test_command -vv)
    # Without WORKING_DIRECTORY the `add_test` prints better diagnostic info
    add_test(NAME "${testsuite_test_name}" COMMAND ${testsuite_test_command})
    if(ARG_TEST_ENV)
        set_tests_properties("${testsuite_test_name}" PROPERTIES ENVIRONMENT "${ARG_TEST_ENV}")
    endif()

    _userver_get_test_resource_locks_for_target("${ARG_SERVICE_TARGET}" testsuite_resource_locks)
    list(APPEND testsuite_resource_locks ${ARG_RESOURCE_LOCKS})
    if(testsuite_resource_locks)
        list(REMOVE_DUPLICATES testsuite_resource_locks)
        set_tests_properties("${testsuite_test_name}" PROPERTIES RESOURCE_LOCK "${testsuite_resource_locks}")
    endif()

    # Pre-collect command in a list to support spaces in paths
    set(testsuite_start_command)
    list(APPEND testsuite_start_command "${python_binary}")
    list(APPEND testsuite_start_command "${TESTSUITE_RUNNER}")
    list(APPEND testsuite_start_command ${PRETTY_LOGS_MODE})
    list(APPEND testsuite_start_command --service-runner-mode)
    list(APPEND testsuite_start_command -vvs)
    add_custom_target(
        "start-${service_target_with_suffix}"
        COMMAND ${testsuite_start_command}
        DEPENDS "${TESTSUITE_RUNNER}"
        VERBATIM USES_TERMINAL
    )
    add_dependencies("start-${service_target_with_suffix}" "${ARG_SERVICE_TARGET}")
endfunction()

# Tries to search service files in some standard places. Should be invoked from the service's CMakeLists.txt Supports
# the following file structure (and a few others):
#
# * configs/config.yaml
# * configs/config_vars.[testsuite|tests].yaml [optional]
# * configs/dynamic_config_fallback.json [optional]
# * configs/[secdist|secure_data].json [optional]
# * [testsuite|tests]/conftest.py
#
# @param SERVICE_TARGET
# @param TEST_SUFFIX
# @param WORKING_DIRECTORY
# @param PYTHON_BINARY
# @param PRETTY_LOGS
# @param CONFIG_PATH
# @param CONFIG_VARS_PATH
# @param DYNAMIC_CONFIG_FALLBACK_PATH
# @param SECDIST_PATH
# @param DUMP_CONFIG
# @param SQL_LIBRARY
# @multiparam PYTEST_ARGS
# @multiparam REQUIREMENTS
# @multiparam PYTHONPATH
# @multiparam TEST_ENV
# @multiparam RESOURCE_LOCKS ctest resource locks for databases the service uses
#   without linking their module (e.g. 'userver_postgresql' for an odbc service)
function(userver_testsuite_add_simple)
    set(oneValueArgs
        SERVICE_TARGET
        TEST_SUFFIX
        WORKING_DIRECTORY
        PYTHON_BINARY
        PRETTY_LOGS
        CONFIG_PATH
        CONFIG_VARS_PATH
        DYNAMIC_CONFIG_FALLBACK_PATH
        SECDIST_PATH
        DUMP_CONFIG
        SQL_LIBRARY
    )
    set(multiValueArgs PYTEST_ARGS REQUIREMENTS PYTHONPATH TEST_ENV RESOURCE_LOCKS)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    _userver_setup_environment_validate_impl()

    set(pytest_additional_args)

    if(ARG_WORKING_DIRECTORY)
        if(IS_ABSOLUTE "${ARG_WORKING_DIRECTORY}")
            file(RELATIVE_PATH tests_relative_path "${CMAKE_CURRENT_SOURCE_DIR}" "${ARG_WORKING_DIRECTORY}")
        else()
            set(tests_relative_path "${ARG_WORKING_DIRECTORY}")
            get_filename_component(
                ARG_WORKING_DIRECTORY "${ARG_WORKING_DIRECTORY}" REALPATH BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
            )
        endif()
    else()
        # Probe the standard test-directory candidates; keep both ARG_WORKING_DIRECTORY
        # (absolute dir) and tests_relative_path (relative name) in sync.
        foreach(probable_tests_path IN ITEMS "testsuite" "tests" ".")
            if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${probable_tests_path}/conftest.py")
                set(ARG_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${probable_tests_path}")
                set(tests_relative_path "${probable_tests_path}")
                break()
            endif()
        endforeach()
    endif()

    if(NOT ARG_SERVICE_TARGET)
        set(ARG_SERVICE_TARGET "${PROJECT_NAME}")
    endif()
    if(NOT ARG_TEST_SUFFIX)
        set(ARG_TEST_SUFFIX "${tests_relative_path}")
    endif()
    if("${ARG_TEST_SUFFIX}" STREQUAL "."
       OR "${ARG_TEST_SUFFIX}" STREQUAL "tests"
       OR "${ARG_TEST_SUFFIX}" STREQUAL "testsuite"
    )
        set(ARG_TEST_SUFFIX "")
    endif()

    set(DUMP_CONFIG_OPTION "")
    if(ARG_DUMP_CONFIG)
        set(DUMP_CONFIG_OPTION "--dump-config")
    endif()

    if(ARG_CONFIG_PATH)
        get_filename_component(config_path "${ARG_CONFIG_PATH}" REALPATH BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    elseif(ARG_DUMP_CONFIG)
        set(config_path "${CMAKE_CURRENT_BINARY_DIR}/Testing/Temporary/static_config.yaml")
    else()
        _userver_find_first_existing(
            config_path "${CMAKE_CURRENT_SOURCE_DIR}/configs/static_config.yaml"
            "${CMAKE_CURRENT_SOURCE_DIR}/configs/config.yaml" "${CMAKE_CURRENT_SOURCE_DIR}/static_config.yaml"
            "${CMAKE_CURRENT_SOURCE_DIR}/config.yaml"
        )
        if(NOT config_path)
            message(FATAL_ERROR "Failed to find service static config for testsuite. "
                                "Please pass it to ${CMAKE_CURRENT_FUNCTION} as CONFIG_PATH arg."
            )
        endif()
    endif()

    if(ARG_CONFIG_VARS_PATH)
        get_filename_component(
            config_vars_path "${ARG_CONFIG_VARS_PATH}" REALPATH BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
        )
    else()
        _userver_find_first_existing(
            config_vars_path
            "${CMAKE_CURRENT_SOURCE_DIR}/configs/config_vars.testsuite.yaml"
            "${CMAKE_CURRENT_SOURCE_DIR}/configs/config_vars.testing.yaml"
            "${CMAKE_CURRENT_SOURCE_DIR}/configs/config_vars.yaml"
            "${CMAKE_CURRENT_SOURCE_DIR}/config_vars.testsuite.yaml"
            "${CMAKE_CURRENT_SOURCE_DIR}/config_vars.testing.yaml"
            "${CMAKE_CURRENT_SOURCE_DIR}/config_vars.yaml"
        )
    endif()
    if(config_vars_path)
        list(APPEND pytest_additional_args "--service-config-vars=${config_vars_path}")
    endif()

    if(ARG_DYNAMIC_CONFIG_FALLBACK_PATH)
        get_filename_component(
            dynamic_config_fallback_path "${ARG_DYNAMIC_CONFIG_FALLBACK_PATH}" REALPATH BASE_DIR
            "${CMAKE_CURRENT_SOURCE_DIR}"
        )
    else()
        _userver_find_first_existing(
            dynamic_config_fallback_path "${CMAKE_CURRENT_SOURCE_DIR}/configs/dynamic_config_fallback.json"
            "${CMAKE_CURRENT_SOURCE_DIR}/dynamic_config_fallback.json"
        )
    endif()
    if(dynamic_config_fallback_path)
        list(APPEND pytest_additional_args "--config-fallback=${dynamic_config_fallback_path}")
    endif()

    if(ARG_SECDIST_PATH)
        # BUG FIX (was: ARG_CONFIG_VARS_PATH — copy-paste from the config_vars branch above).
        # The helper extraction structurally prevents this class of error from recurring.
        get_filename_component(secdist_path "${ARG_SECDIST_PATH}" REALPATH BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    else()
        _userver_find_first_existing(
            secdist_path "${CMAKE_CURRENT_SOURCE_DIR}/configs/secdist.json"
            "${CMAKE_CURRENT_SOURCE_DIR}/configs/secure_data.json" "${CMAKE_CURRENT_SOURCE_DIR}/secdist.json"
            "${CMAKE_CURRENT_SOURCE_DIR}/secure_data.json"
        )
    endif()
    if(secdist_path)
        list(APPEND pytest_additional_args "--service-secdist=${secdist_path}")
    endif()

    if(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/proto")
        list(APPEND ARG_PYTHONPATH "${CMAKE_CURRENT_BINARY_DIR}/proto")
    endif()

    userver_testsuite_add(
        SERVICE_TARGET "${ARG_SERVICE_TARGET}"
        TEST_SUFFIX "${ARG_TEST_SUFFIX}"
        WORKING_DIRECTORY "${ARG_WORKING_DIRECTORY}"
        PYTHON_BINARY "${ARG_PYTHON_BINARY}"
        PRETTY_LOGS "${ARG_PRETTY_LOGS}"
        PYTEST_ARGS
            "--service-config=${config_path}" "--service-source-dir=${CMAKE_CURRENT_SOURCE_DIR}"
            "--service-binary=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}" "${DUMP_CONFIG_OPTION}"
            ${pytest_additional_args} ${ARG_PYTEST_ARGS}
        REQUIREMENTS ${ARG_REQUIREMENTS}
        PYTHONPATH ${ARG_PYTHONPATH}
        TEST_ENV ${ARG_TEST_ENV}
        RESOURCE_LOCKS ${ARG_RESOURCE_LOCKS}
        SQL_LIBRARY ${ARG_SQL_LIBRARY}
    )
endfunction()

# Add utest, test runs in testsuite env.
#
# @option DISABLE_GTEST_XML_OUTPUT
# @param oneValueArgs
# @multiparam DATABASES
# @multiparam TEST_ENV
# @multiparam TEST_ARGS
function(userver_add_utest)
    set(options DISABLE_GTEST_XML_OUTPUT)
    set(oneValueArgs NAME)
    set(multiValueArgs DATABASES TEST_ENV TEST_ARGS)

    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT USERVER_FEATURE_TESTSUITE)
        message(FATAL_ERROR "userver_add_utest requires 'USERVER_FEATURE_TESTSUITE=ON'")
    endif()

    get_property(USERVER_TESTSUITE_DIR GLOBAL PROPERTY userver_testsuite_dir)

    if(ARG_DATABASES)
        # Provision a per-database-set venv so that the testsuite environment can start
        # the required DB daemons. The venv is named "utest-<sorted-dbs>" and is UNIQUE,
        # meaning tests with the same database set share one venv and it is built only once.
        #
        # This is done here, at userver_add_utest time, rather than eagerly in
        # _userver_prepare_testsuite, because all userver::<db> targets are guaranteed to
        # exist by the time userver_add_utest is called (find_package has completed), while
        # _userver_prepare_testsuite runs during the core component config load, before any
        # DB component targets are created.
        _userver_ensure_testsuite_venv_for_databases(
            env_script utest_db_python VENV_PREFIX utest DATABASES ${ARG_DATABASES}
        )

        list(JOIN ARG_DATABASES "," databases_value)
        set(additional_args "--databases=${databases_value}")
    else()
        set(env_script "${CMAKE_BINARY_DIR}/testsuite/env")
        set(additional_args "--databases=")
    endif()

    if(NOT ARG_DISABLE_GTEST_XML_OUTPUT)
        list(APPEND ARG_TEST_ARGS "--gtest_output=xml:${CMAKE_BINARY_DIR}/test-results/${ARG_NAME}.xml")
    endif()

    add_test(NAME "${ARG_NAME}" COMMAND "${env_script}" ${additional_args} run -- $<TARGET_FILE:${ARG_NAME}>
                                        ${ARG_TEST_ARGS}
    )
    if(ARG_TEST_ENV)
        set_tests_properties("${ARG_NAME}" PROPERTIES ENVIRONMENT "${ARG_TEST_ENV}")
    endif()

    set(utest_resource_locks "")
    foreach(database IN LISTS ARG_DATABASES)
        list(APPEND utest_resource_locks "userver_${database}")
    endforeach()
    if(utest_resource_locks)
        set_tests_properties("${ARG_NAME}" PROPERTIES RESOURCE_LOCK "${utest_resource_locks}")
    endif()
endfunction()

# @param NAME
# @multiparam DATABASES
# @multiparam TEST_ENV
function(userver_add_ubench_test)
    set(options)
    set(oneValueArgs NAME)
    set(multiValueArgs DATABASES TEST_ENV)

    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(USERVER_CONAN)
        set(BENCHMARK_VERSION ${benchmark_VERSION})
    else()
        set(BENCHMARK_VERSION ${UserverGBench_VERSION})
    endif()

    if(BENCHMARK_VERSION VERSION_LESS "1.8.0")
        set(BENCHMARK_MIN_TIME "0")
    else()
        set(BENCHMARK_MIN_TIME "0.0s")
    endif()

    userver_add_utest(
        NAME "${ARG_NAME}"
        DATABASES ${ARG_DATABASES}
        TEST_ENV ${ARG_TEST_ENV}
        TEST_ARGS --benchmark_min_time=${BENCHMARK_MIN_TIME} --benchmark_color=no
        DISABLE_GTEST_XML_OUTPUT ON
    )
endfunction()

# Converts a target name into a ';'-separated list of ctest RESOURCE_LOCK names, one
# per userver database module the target links against (directly or transitively).
# Tests that share a lock are never run concurrently by ctest, which protects the
# per-engine database daemons started by testsuite (each on a fixed port/data-dir)
# from races during parallel `ctest -j`. Lock name for an engine is "userver_<engine>".
#
# The set of engines is driven entirely by the DI registry populated via
# userver_testsuite_register_database (called from each UserverTestsuiteDb-<mod>.cmake
# fragment). Embedded engines without a shared daemon (e.g. sqlite, rocksdb) are
# naturally omitted because they are never registered.
function(_userver_get_test_resource_locks_for_target target output_var)
    set(resource_locks "")

    _userver_testsuite_active_databases(active_dbs TARGET "${target}")
    foreach(db IN LISTS active_dbs)
        list(APPEND resource_locks "userver_${db}")
    endforeach()

    set(${output_var}
        "${resource_locks}"
        PARENT_SCOPE
    )
endfunction()

_userver_prepare_testsuite()
