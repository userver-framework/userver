# Functions for setting up Python virtual environments.
#
# Provides:
# - USERVER_PYTHON_PATH option
# - USERVER_PIP_USE_SYSTEM_PACKAGES option
# - USERVER_PIP_OPTIONS option
# - userver_venv_setup function that sets up a Python virtual environment
#   with the given requirements
#
# Implementation note: public functions here should be usable even without
# a direct include of this script, so the functions should not rely
# on non-cache variables being present.
include_guard(GLOBAL)

function(_userver_prepare_venv_variables)
  set(USERVER_PYTHON_PATH "python3" CACHE FILEPATH "Path to python3 executable to use")
  message(STATUS "Python: ${USERVER_PYTHON_PATH}")
  option(
      USERVER_PIP_USE_SYSTEM_PACKAGES
      "Use system python packages inside venv"
      OFF
  )
  set(USERVER_PIP_OPTIONS "" CACHE STRING "Options for all pip calls")
endfunction()

_userver_prepare_venv_variables()

function(_userver_append_requirements_from_file output_variable)
  set(venv_requirements "")
  foreach(requirement IN LISTS ARGN)
    file(READ "${requirement}" requirement_contents)
    if(NOT requirement_contents MATCHES "\n$")
      message(FATAL_ERROR "venv requirements file must end with a newline")
    endif()
    string(APPEND venv_requirements "${requirement_contents}")
  endforeach()

  # Remove duplicates and comments
  string(REPLACE "\n" ";" venv_requirements_list ${venv_requirements})
  list(FILTER venv_requirements_list EXCLUDE REGEX "[ \t\r\n]*#.*")
  list(SORT venv_requirements_list)
  list(REMOVE_DUPLICATES venv_requirements_list)
  string(REPLACE ";" "\n" venv_requirements "${venv_requirements_list}")

  set(${output_variable} ${venv_requirements} PARENT_SCOPE)
endfunction()

function(userver_venv_setup)
  set(options UNIQUE)
  set(oneValueArgs NAME PYTHON_OUTPUT_VAR)
  set(multiValueArgs REQUIREMENTS PIP_ARGS)

  cmake_parse_arguments(
      ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" "${ARGN}")

  _userver_setup_environment_validate_impl()

  if(NOT ARG_REQUIREMENTS)
    message(FATAL_ERROR
        "In 'userver_venv_setup' provide a REQUIREMENTS option with a filepath "
        "to file with all the requirements for the virtual env being created."
    )
    return()
  endif()

  list(APPEND ARG_PIP_ARGS ${USERVER_PIP_OPTIONS})
  set(venv_params "")
  set(format_version 2)
  string(APPEND venv_params "format-version=${format_version}\n")
  string(APPEND venv_params "pip-args=${ARG_PIP_ARGS}\n")
  _userver_append_requirements_from_file(venv_params ${ARG_REQUIREMENTS})

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

  set(venv_additional_args)
  if(USERVER_PIP_USE_SYSTEM_PACKAGES)
    list(APPEND venv_additional_args "--system-site-packages")
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
        venv ${ARG_REQUIREMENTS} ${venv_additional_args} ${ARG_PIP_ARGS})
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

  message(STATUS "Setting up the venv at ${venv_dir}")

  if(NOT EXISTS "${venv_dir}")
    execute_process(
        COMMAND
        "${USERVER_PYTHON_PATH}"
        -m venv
        "${venv_dir}"
        ${venv_additional_args}
        RESULT_VARIABLE status
    )
    if(status)
      file(REMOVE_RECURSE "${venv_dir}")
      message(FATAL_ERROR
          "Failed to create Python virtual environment. "
          "On Debian-based systems, venv is installed separately:\n"
          "sudo apt install python3-venv"
      )
    endif()
  endif()

  # If pip has already installed packages using the same requirements,
  # then don't run it again. This optimization dramatically reduces
  # re-Configure times.
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
        "${venv_bin_dir}/python3" -m pip install
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

