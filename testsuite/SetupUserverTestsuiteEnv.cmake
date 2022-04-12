# /// [testsuite - UserverTestsuite]
include(UserverTestsuite)
# /// [testsuite - UserverTestsuite]

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(TESTSUITE_REQUIREMENTS ${CMAKE_SOURCE_DIR}/testsuite/requirements-macos.txt)
else()
  set(TESTSUITE_REQUIREMENTS ${CMAKE_SOURCE_DIR}/testsuite/requirements.txt)
endif()

userver_venv_setup(
  NAME userver-testenv
  PYTHON_OUTPUT_VAR TESTSUITE_PYTHON_BINARY
  REQUIREMENTS ${TESTSUITE_REQUIREMENTS}
)
