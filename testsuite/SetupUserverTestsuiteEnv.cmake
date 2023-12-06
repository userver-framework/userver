# This script sets up venv for tests of userver itself.
# It is not used for testing services based on userver.

# /// [testsuite - UserverTestsuite]
include(UserverTestsuite)
# /// [testsuite - UserverTestsuite]

list(APPEND TESTSUITE_REQUIREMENTS
  ${USERVER_ROOT_DIR}/testsuite/requirements.txt)

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
    message(FATAL_ERROR "SetupProtobuf should be run before this cmake file")
  endif()
endif()

if(USERVER_FEATURE_MONGODB)
  list(APPEND TESTSUITE_REQUIREMENTS
    ${USERVER_ROOT_DIR}/testsuite/requirements-mongo.txt)
  list(APPEND TESTSUITE_MODULES mongodb)
  message(STATUS "Add mongodb python depends")
endif()

if(USERVER_FEATURE_POSTGRESQL)
  list(APPEND TESTSUITE_REQUIREMENTS
    ${USERVER_ROOT_DIR}/testsuite/requirements-postgres.txt)
  if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    list(APPEND TESTSUITE_MODULES postgresql-binary)
  else()
    list(APPEND TESTSUITE_MODULES postgresql)
  endif()
  message(STATUS "Add postgresql python depends")
endif()

if(USERVER_FEATURE_YDB)
  list(APPEND TESTSUITE_REQUIREMENTS
    ${USERVER_ROOT_DIR}/testsuite/requirements-ydb.txt)
message(STATUS "Add YDB python depends")
endif()

if(USERVER_FEATURE_REDIS)
  list(APPEND TESTSUITE_REQUIREMENTS
    ${USERVER_ROOT_DIR}/testsuite/requirements-redis.txt)
  list(APPEND TESTSUITE_MODULES redis)
  message(STATUS "Add redis python depends")
endif()

if(USERVER_FEATURE_CLICKHOUSE)
  list(APPEND TESTSUITE_MODULES clickhouse)
  message(STATUS "Add clickhouse python depends")
endif()

if(USERVER_FEATURE_RABBITMQ)
  list(APPEND TESTSUITE_MODULES rabbitmq)
  message(STATUS "Add rabbitmq python depends")
endif()

if(USERVER_FEATURE_MYSQL)
  list(APPEND TESTSUITE_MODULES mysql)
  message(STATUS "Add mysql python depends")
endif()

file(READ ${USERVER_ROOT_DIR}/testsuite/requirements-testsuite.txt TESTSUITE_TXT)
if(TESTSUITE_MODULES)
  list(JOIN TESTSUITE_MODULES "," TESTSUITE_MODULES_STR)
  string(REPLACE "yandex-taxi-testsuite[]" "yandex-taxi-testsuite[${TESTSUITE_MODULES_STR}]" TESTSUITE_TXT ${TESTSUITE_TXT})
  message(STATUS "set testsuite with modules: ${TESTSUITE_TXT}")
endif()
file(WRITE ${CMAKE_BINARY_DIR}/requirements-testsuite.txt ${TESTSUITE_TXT})
list(APPEND TESTSUITE_REQUIREMENTS
  ${CMAKE_BINARY_DIR}/requirements-testsuite.txt)

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
