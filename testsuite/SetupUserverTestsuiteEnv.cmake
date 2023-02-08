# /// [testsuite - UserverTestsuite]
include(UserverTestsuite)
# /// [testsuite - UserverTestsuite]

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(TESTSUITE_REQUIREMENTS ${USERVER_ROOT_DIR}/testsuite/requirements-macos.txt)
else()
  set(TESTSUITE_REQUIREMENTS ${USERVER_ROOT_DIR}/testsuite/requirements.txt)
endif()

if (USERVER_FEATURE_GRPC)
  list(APPEND TESTSUITE_REQUIREMENTS
    ${USERVER_ROOT_DIR}/testsuite/requirements-grpc.txt)
endif()

userver_venv_setup(
  NAME userver-testenv
  PYTHON_OUTPUT_VAR TESTSUITE_PYTHON_BINARY
  REQUIREMENTS ${TESTSUITE_REQUIREMENTS}
)
