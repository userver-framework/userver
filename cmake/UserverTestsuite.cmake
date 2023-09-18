include_guard()

include(CTest)
include(FindPython)

option(USERVER_FEATURE_TESTSUITE "Enable functional tests via testsuite" ON)

if(USERVER_FEATURE_TESTSUITE)
  get_property(userver_python_dev_checked
      GLOBAL PROPERTY userver_python_dev_checked)
  if(NOT userver_python_dev_checked)
    # find package python3-dev required by virtualenv
    execute_process(
        COMMAND bash "-c" "command -v python3-config"
        OUTPUT_VARIABLE PYTHONCONFIG_FOUND
    )
    if(NOT PYTHONCONFIG_FOUND)
      message(FATAL_ERROR "Python dev is not found")
    endif()
    set_property(GLOBAL PROPERTY userver_python_dev_checked "TRUE")
  endif()
endif()

get_filename_component(
    USERVER_TESTSUITE_DIR "${CMAKE_CURRENT_LIST_DIR}/../testsuite" ABSOLUTE)

function(userver_venv_setup)
  set(options UNIQUE)
  set(oneValueArgs NAME PYTHON_OUTPUT_VAR)
  set(multiValueArgs REQUIREMENTS VIRTUALENV_ARGS PIP_ARGS)

  cmake_parse_arguments(
      ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" "${ARGN}")

  if(NOT ARG_REQUIREMENTS)
    message(FATAL_ERROR "No REQUIREMENTS given for venv")
    return()
  endif()

  if(NOT ARG_NAME)
    set(venv_name "venv")
    set(python_output_var "TESTSUITE_VENV_PYTHON")
  else()
    set(venv_name "venv-${ARG_NAME}")
    string(TOUPPER "TESTSUITE_VENV_${ARG_NAME}_PYTHON" python_output_var)
  endif()

  if(ARG_PYTHON_OUTPUT_VAR)
    set(python_output_var "${ARG_PYTHON_OUTPUT_VAR}")
  endif()

  if(ARG_UNIQUE)
    set(parent_directory "${CMAKE_BINARY_DIR}")
  else()
    set(parent_directory "${CMAKE_CURRENT_BINARY_DIR}")
  endif()

  set(venv_dir "${parent_directory}/${venv_name}")
  set(venv_bin_dir "${venv_dir}/bin")
  set("${python_output_var}" "${venv_bin_dir}/python" PARENT_SCOPE)

  # A unique venv is set up once for the whole build.
  # For example, a userver gRPC cmake script may be included multiple times
  # during the Configure, but only 1 venv should be created.
  # Global properties are used to check that all userver_venv_setup
  # for a given venv are invoked with the same params.
  if(ARG_UNIQUE)
    set(venv_unique_params
        venv ${ARG_REQUIREMENTS} ${ARG_VIRTUALENV_ARGS} ${ARG_PIP_ARGS})
    get_property(cached_venv_unique_params
        GLOBAL PROPERTY "userver-venv-${ARG_NAME}-params")
    if(cached_venv_unique_params)
      if(NOT cached_venv_unique_params STREQUAL venv_unique_params)
        message(FATAL_ERROR
            "Unique venv '${ARG_NAME}' is created multiple times with "
            "different params, "
            "before='${cached_venv_unique_params}' "
            "after='${venv_unique_params}'")
      endif()
      return()
    endif()
  endif()

  find_program(TESTSUITE_VIRTUALENV virtualenv)
  if(NOT TESTSUITE_VIRTUALENV)
    message(FATAL_ERROR
        "No virtualenv binary found, try to install:\n"
        "Debian: sudo apt install virtualenv\n"
        "MacOS: brew install virtualenv\n"
        "ArchLinux: sudo pacman -S python-virtualenv")
  endif()

  message(STATUS "Setting up the virtualenv at ${venv_dir}")

  if(NOT EXISTS "${venv_dir}")
    execute_process(
        COMMAND
        "${TESTSUITE_VIRTUALENV}"
        --system-site-packages
        "--python=${PYTHON}"
        "${venv_dir}"
        ${ARG_VIRTUALENV_ARGS}
        RESULT_VARIABLE status
    )
    if(status)
      file(REMOVE_RECURSE "${venv_dir}")
      message(FATAL_ERROR "Failed to create Python virtual environment")
    endif()
  endif()

  # If pip has already installed packages using the same requirements,
  # then don't run it again. This optimization dramatically reduces
  # re-Configure times.
  set(venv_params "")
  set(format_version 1)
  string(APPEND venv_params "format-version=${format_version}\n")
  string(APPEND venv_params "pip-args=${ARG_PIP_ARGS}\n")
  foreach(requirement IN LISTS ARG_REQUIREMENTS)
    file(READ "${requirement}" requirement_contents)
    if(NOT requirement_contents MATCHES "\n$")
      message(FATAL_ERROR "venv requirements file must end with a newline")
    endif()
    string(APPEND venv_params "${requirement_contents}")
  endforeach()

  set(should_run_pip TRUE)
  set(venv_params_file "${venv_dir}/venv-params.txt")
  if(EXISTS "${venv_params_file}")
    file(READ "${venv_params_file}" venv_params_old)
    if(venv_params_old STREQUAL venv_params)
      set(should_run_pip FALSE)
    endif()
  endif()

  if(should_run_pip)
    message(STATUS "Installing requirements:")
    foreach(requirement IN LISTS ARG_REQUIREMENTS)
      message(STATUS "  ${requirement}")
    endforeach()
    list(
        TRANSFORM ARG_REQUIREMENTS
        PREPEND "--requirement="
        OUTPUT_VARIABLE pip_requirements
    )
    execute_process(
        COMMAND
        "${venv_bin_dir}/pip" install
        --disable-pip-version-check
        -U ${pip_requirements}
        ${ARG_PIP_ARGS}
        RESULT_VARIABLE status
    )
    if(status)
      message(FATAL_ERROR "Failed to install venv requirements")
    endif()
    file(WRITE "${venv_params_file}" "${venv_params}")
  endif()

  if(ARG_UNIQUE)
    set_property(GLOBAL PROPERTY "userver-venv-${ARG_NAME}-params"
        ${venv_unique_params})
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
