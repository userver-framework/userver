# This script sets up venv for tests of userver itself. It is not used for testing services based on userver.

userver_testsuite_requirements(REQUIREMENTS_FILES_VAR requirements_files)

# Log which DB extras will be baked into the shared userver-testenv venv so that
# a missing module (e.g. "No module named 'redis'") is easy to diagnose from the
# CMake configure log.
_userver_testsuite_active_databases(_userver_testenv_active_dbs)
if(_userver_testenv_active_dbs)
    message(STATUS "userver-testenv: active DB extras: ${_userver_testenv_active_dbs}")
else()
    message(
        STATUS
            "userver-testenv: no DB extras detected (all USERVER_FEATURE_* are OFF or DB fragments not yet registered)"
    )
endif()
unset(_userver_testenv_active_dbs)

list(APPEND requirements_files "${USERVER_ROOT_DIR}/testsuite/requirements-internal-tests.txt")

userver_venv_setup(
    NAME userver-testenv
    PYTHON_OUTPUT_VAR TESTSUITE_PYTHON_BINARY
    REQUIREMENTS ${requirements_files}
)

function(userver_chaos_testsuite_add)
    set(options)
    set(oneValueArgs TESTS_DIRECTORY)
    set(multiValueArgs PYTHONPATH ENV RESOURCE_LOCKS)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    userver_testsuite_add_simple(
        WORKING_DIRECTORY "${ARG_TESTS_DIRECTORY}"
        PYTHON_BINARY "${TESTSUITE_PYTHON_BINARY}"
        PYTHONPATH ${ARG_PYTHONPATH}
        TEST_ENV "${ARG_ENV}"
        RESOURCE_LOCKS ${ARG_RESOURCE_LOCKS}
    )
endfunction()
