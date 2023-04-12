include(CTest)
include(FindPython)

option(USERVER_FEATURE_TESTSUITE "Enable functional tests via testsuite" ON)
if (USERVER_FEATURE_TESTSUITE) 
  # find package python3-dev required by virtualenv
  execute_process(
    COMMAND bash "-c" "command -v python3-config"
    OUTPUT_VARIABLE PYTHONCONFIG_FOUND
  )
  if (NOT PYTHONCONFIG_FOUND)
    message(FATAL_ERROR "Python dev is not found")
  endif()
endif()

get_filename_component(
  USERVER_TESTSUITE_DIR ${CMAKE_CURRENT_LIST_DIR}/../testsuite ABSOLUTE)

function(userver_venv_setup)
  set(options)
  set(oneValueArgs NAME PYTHON_OUTPUT_VAR)
  set(multiValueArgs REQUIREMENTS VIRTUALENV_ARGS PIP_ARGS)

  cmake_parse_arguments(
    ARG "${options}" "${oneValueArgs}" "${multiValueArgs}"  ${ARGN})

  if (NOT ARG_REQUIREMENTS)
    message(FATAL_ERROR "No REQUIREMENTS given for venv")
    return()
  endif()

  if (NOT ARG_NAME)
    set(VENV_NAME "venv")
    set(PYTHON_OUTPUT_VAR "TESTSUITE_VENV_PYTHON")
  else()
    set(VENV_NAME "venv-${ARG_NAME}")
    string(TOUPPER "TESTSUITE_VENV_${ARG_NAME}_PYTHON" PYTHON_OUTPUT_VAR)
  endif()

  if (ARG_PYTHON_OUTPUT_VAR)
    set(PYTHON_OUTPUT_VAR ${ARG_PYTHON_OUTPUT_VAR})
  endif()

  find_program(TESTSUITE_VIRTUALENV virtualenv)
  if (${TESTSUITE_VIRTUALENV} STREQUAL "TESTSUITE_VIRTUALENV-NOTFOUND")
    message(FATAL_ERROR
      "No virtualenv binary found, try to install:\n"
      "Debian: sudo apt install virtualenv\n"
      "MacOS: brew install virtualenv")
  endif()

  set(VENV_DIR ${CMAKE_CURRENT_BINARY_DIR}/${VENV_NAME})
  set(VENV_BIN_DIR ${VENV_DIR}/bin)
  set(${PYTHON_OUTPUT_VAR} ${VENV_BIN_DIR}/python PARENT_SCOPE)

  message(STATUS "Setting up the virtualenv with requirements:")
  foreach(req ${ARG_REQUIREMENTS})
    message(STATUS "  ${req}")
  endforeach()

  if (NOT EXISTS ${VENV_DIR})
    execute_process(
      COMMAND ${TESTSUITE_VIRTUALENV} --system-site-packages --python=${PYTHON} ${VENV_DIR} ${ARG_VIRTUALENV_ARGS}
      RESULT_VARIABLE STATUS
    )
    if (STATUS)
      file(REMOVE_RECURSE ${VENV_DIR})
      message(FATAL_ERROR "Failed to create Python virtual environment")
    endif()
  endif()
  list(TRANSFORM ARG_REQUIREMENTS PREPEND "--requirement="
    OUTPUT_VARIABLE PIP_REQUIREMENTS)
  execute_process(
    COMMAND ${VENV_BIN_DIR}/pip install --disable-pip-version-check
      -U ${PIP_REQUIREMENTS} ${ARG_PIP_ARGS}
    RESULT_VARIABLE STATUS
  )
  if (STATUS)
    message(FATAL_ERROR "Failed to install testsuite dependencies")
  endif()
endfunction()

function(userver_testsuite_add)
  set(options)
  set(oneValueArgs SERVICE_TARGET WORKING_DIRECTORY PYTHON_BINARY PRETTY_LOGS)
  set(multiValueArgs PYTEST_ARGS REQUIREMENTS PYTHONPATH VIRTUALENV_ARGS)
  cmake_parse_arguments(
    ARG "${options}" "${oneValueArgs}" "${multiValueArgs}"  ${ARGN})

  if (NOT ARG_SERVICE_TARGET)
    message(FATAL_ERROR "No SERVICE_TARGET given for testsuite")
    return()
  endif()

  if (NOT ARG_WORKING_DIRECTORY)
    set(ARG_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  endif()

  if (NOT DEFINED ARG_PRETTY_LOGS)
    set(ARG_PRETTY_LOGS ON)
  endif()

  set(TESTSUITE_TARGET "testsuite-${ARG_SERVICE_TARGET}")

  if (NOT USERVER_FEATURE_TESTSUITE)
    message(STATUS "Testsuite target ${TESTSUITE_TARGET} is disabled")
    return()
  endif()

  if (ARG_REQUIREMENTS)
    userver_venv_setup(
      NAME ${TESTSUITE_TARGET}
      REQUIREMENTS ${ARG_REQUIREMENTS}
      PYTHON_OUTPUT_VAR PYTHON_BINARY
      VIRTUALENV_ARGS ${ARG_VIRTUALENV_ARGS}
    )
  elseif (ARG_PYTHON_BINARY)
    set(PYTHON_BINARY "${ARG_PYTHON_BINARY}")
  else()
    set(PYTHON_BINARY "${TESTSUITE_VENV_PYTHON}")
  endif()

  if (NOT PYTHON_BINARY)
    message(FATAL_ERROR "No python binary given.")
  endif()

  set(TESTSUITE_RUNNER "${CMAKE_CURRENT_BINARY_DIR}/runtests-${TESTSUITE_TARGET}")
  list(APPEND ARG_PYTHONPATH ${USERVER_TESTSUITE_DIR}/pytest_plugins)

  execute_process(
    COMMAND
    ${PYTHON_BINARY} ${USERVER_TESTSUITE_DIR}/create_runner.py
    -o ${TESTSUITE_RUNNER}
    --python=${PYTHON_BINARY}
    "--python-path=${ARG_PYTHONPATH}"
    --
    --build-dir=${CMAKE_BINARY_DIR}
    ${ARG_PYTEST_ARGS}
    RESULT_VARIABLE STATUS
  )
  if (STATUS)
    message(FATAL_ERROR "Failed to create testsuite runner")
  endif()

  set(PRETTY_LOGS_MODE "")
  if (ARG_PRETTY_LOGS)
      set(PRETTY_LOGS_MODE "--service-logs-pretty")
  endif()

  # Without WORKING_DIRECTORY the `add_test` prints better diagnostic info
  add_test(
    NAME ${TESTSUITE_TARGET}
    COMMAND ${TESTSUITE_RUNNER} ${PRETTY_LOGS_MODE} -vv ${ARG_WORKING_DIRECTORY}
  )

  add_custom_target(
    start-${ARG_SERVICE_TARGET}
    COMMAND ${TESTSUITE_RUNNER} --service-runner-mode ${PRETTY_LOGS_MODE} -vvs ${ARG_WORKING_DIRECTORY}
    DEPENDS ${TESTSUITE_RUNNER} ${ARG_SERVICE_TARGET}
  )
endfunction()
