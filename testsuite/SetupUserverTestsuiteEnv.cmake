# This script sets up venv for tests of userver itself.
# It is not used for testing services based on userver.

# /// [testsuite - UserverTestsuite]
include(UserverTestsuite)
# /// [testsuite - UserverTestsuite]

list(APPEND TESTSUITE_REQUIREMENTS
  ${USERVER_ROOT_DIR}/testsuite/requirements.txt)

if(USERVER_FEATURE_GRPC)
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
    message(FATAL_ERROR "SetupProtobuf should be run before this cmake file")
  endif()
endif()

if(USERVER_FEATURE_MONGODB)
  list(APPEND TESTSUITE_REQUIREMENTS
    ${USERVER_ROOT_DIR}/testsuite/requirements-mongo.txt)
endif()

if(USERVER_FEATURE_POSTGRESQL)
  if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    list(APPEND TESTSUITE_REQUIREMENTS
        ${USERVER_ROOT_DIR}/testsuite/requirements-postgres-macos.txt)
  else()
    list(APPEND TESTSUITE_REQUIREMENTS
        ${USERVER_ROOT_DIR}/testsuite/requirements-postgres.txt)
  endif()
endif()

if(USERVER_FEATURE_YDB)
  list(APPEND TESTSUITE_REQUIREMENTS
    ${USERVER_ROOT_DIR}/testsuite/requirements-ydb.txt)
endif()

if(USERVER_FEATURE_REDIS)
  list(APPEND TESTSUITE_REQUIREMENTS
    ${USERVER_ROOT_DIR}/testsuite/requirements-redis.txt)
endif()

if(USERVER_FEATURE_CLICKHOUSE)
  list(APPEND TESTSUITE_REQUIREMENTS
      ${USERVER_ROOT_DIR}/testsuite/requirements-clickhouse.txt)
endif()

if(USERVER_FEATURE_RABBITMQ)
  list(APPEND TESTSUITE_REQUIREMENTS
      ${USERVER_ROOT_DIR}/testsuite/requirements-rabbitmq.txt)
endif()

if(USERVER_FEATURE_MYSQL)
  list(APPEND TESTSUITE_REQUIREMENTS
      ${USERVER_ROOT_DIR}/testsuite/requirements-mysql.txt)
endif()

userver_venv_setup(
  NAME userver-testenv
  PYTHON_OUTPUT_VAR TESTSUITE_PYTHON_BINARY
  REQUIREMENTS ${TESTSUITE_REQUIREMENTS}
)

function(userver_chaos_testsuite_add)
  set(options)
  set(oneValueArgs TESTS_DIRECTORY ENV)
  set(multiValueArgs PYTHONPATH)
  cmake_parse_arguments(
      ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  userver_testsuite_add_simple(
      WORKING_DIRECTORY "${ARG_TESTS_DIRECTORY}"
      PYTHON_BINARY "${TESTSUITE_PYTHON_BINARY}"
      PYTHONPATH ${ARG_PYTHONPATH}
      TEST_ENV "${ARG_ENV}"
  )
endfunction()
