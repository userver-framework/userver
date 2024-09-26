# This script sets up venv for tests of userver itself.
# It is not used for testing services based on userver.

# /// [testsuite - UserverTestsuite]
# cmake
include(UserverTestsuite)
# /// [testsuite - UserverTestsuite]

userver_testsuite_requirements(REQUIREMENTS_FILES_VAR requirements_files)

list(APPEND requirements_files
  "${USERVER_ROOT_DIR}/testsuite/requirements-internal-tests.txt")

userver_venv_setup(
  NAME userver-testenv
  PYTHON_OUTPUT_VAR TESTSUITE_PYTHON_BINARY
  REQUIREMENTS ${requirements_files}
)

function(userver_chaos_testsuite_add)
  set(options)
  set(oneValueArgs TESTS_DIRECTORY)
  set(multiValueArgs PYTHONPATH ENV)
  cmake_parse_arguments(
      ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  userver_testsuite_add_simple(
      WORKING_DIRECTORY "${ARG_TESTS_DIRECTORY}"
      PYTHON_BINARY "${TESTSUITE_PYTHON_BINARY}"
      PYTHONPATH ${ARG_PYTHONPATH}
      TEST_ENV "${ARG_ENV}"
  )
endfunction()
