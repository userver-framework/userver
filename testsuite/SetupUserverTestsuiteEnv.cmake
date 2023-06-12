# /// [testsuite - UserverTestsuite]
include(UserverTestsuite)
# /// [testsuite - UserverTestsuite]

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(TESTSUITE_REQUIREMENTS ${USERVER_ROOT_DIR}/testsuite/requirements-macos.txt)
else()
  set(TESTSUITE_REQUIREMENTS ${USERVER_ROOT_DIR}/testsuite/requirements.txt)
endif()

if (USERVER_FEATURE_GRPC)
  if(Protobuf_FOUND)
    if(Protobuf_VERSION VERSION_GREATER 3.20.0)
      list(APPEND TESTSUITE_REQUIREMENTS
        ${USERVER_ROOT_DIR}/testsuite/requirements-grpc.txt)
    else()
      list(APPEND TESTSUITE_REQUIREMENTS
        ${USERVER_ROOT_DIR}/testsuite/requirements-grpc-old.txt)
      message(STATUS "Forcing old protobuf version for testsuite")
    endif()
  else()
    message(FATAL_ERROR "find_package(Protobuf REQUIRED) should be run before this cmake file")
  endif()
endif()

userver_venv_setup(
  NAME userver-testenv
  PYTHON_OUTPUT_VAR TESTSUITE_PYTHON_BINARY
  REQUIREMENTS ${TESTSUITE_REQUIREMENTS}
)
