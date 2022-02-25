include(UserverTestsuite)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(TESTSUITE_REQUIREMENTS ${CMAKE_CURRENT_SOURCE_DIR}/requirements-macos.txt)
else()
  set(TESTSUITE_REQUIREMENTS ${CMAKE_CURRENT_SOURCE_DIR}/requirements.txt)
endif()

userver_venv_setup(
  NAME userver-testsuite
  PYTHON_OUTPUT_VAR TESTSUITE_PYTHON_BINARY
  REQUIREMENTS ${CMAKE_CURRENT_SOURCE_DIR}/requirements.txt
)

configure_file(env.in env @ONLY)
