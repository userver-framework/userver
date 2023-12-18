include_guard()

include(CTest)
include(FindPython)

option(USERVER_FEATURE_TESTSUITE "Enable functional tests via testsuite" ON)
option(
    USERVER_PIP_USE_SYSTEM_PACKAGES
    "Use system python packages inside virtualenv"
    OFF
)
set(USERVER_PIP_OPTIONS "" CACHE STRING "Options for all pip calls")

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

  if(USERVER_PIP_USE_SYSTEM_PACKAGES)
    list(APPEND ARG_VIRTUALENV_ARGS "--system-site-packages")
  endif()
  list(APPEND ARG_PIP_ARGS ${USERVER_PIP_OPTIONS})

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
        "--python=${USERVER_PYTHON}"
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

function(userver_testsuite_requirements)
  set(options)
  set(oneValueArgs REQUIREMENT_FILES_VAR)
  set(multiValueArgs)

  cmake_parse_arguments(
      ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" "${ARGN}")

  list(APPEND requirements_files
      "${USERVER_TESTSUITE_DIR}/requirements.txt")

  if(USERVER_FEATURE_GRPC OR TARGET userver::grpc)
    if(NOT Protobuf_FOUND)
      if(USERVER_CONAN)
        find_package(Protobuf REQUIRED)
      else()
        include(SetupProtobuf)
      endif()
    endif()
    if(NOT Protobuf_FOUND)
      message(FATAL_ERROR
          "SetupProtobuf should be run before setting up testsuite")
    endif()

    if(Protobuf_VERSION VERSION_GREATER 3.20.0)
      list(APPEND requirements_files
          "${USERVER_TESTSUITE_DIR}/requirements-grpc.txt")
    else()
      list(APPEND requirements_files
          "${USERVER_TESTSUITE_DIR}/requirements-grpc-old.txt")
      message(STATUS "Forcing old protobuf version for testsuite")
    endif()
  endif()

  if(USERVER_FEATURE_MONGODB OR TARGET userver::mongo)
    list(APPEND requirements_files
        "${USERVER_TESTSUITE_DIR}/requirements-mongo.txt")
    list(APPEND testsuite_modules mongodb)
  endif()

  if(USERVER_FEATURE_POSTGRESQL OR TARGET userver::postgresql)
    list(APPEND requirements_files
        "${USERVER_TESTSUITE_DIR}/requirements-postgres.txt")
    if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
      list(APPEND testsuite_modules postgresql-binary)
    else()
      list(APPEND testsuite_modules postgresql)
    endif()
  endif()

  if(USERVER_FEATURE_YDB OR TARGET userver::ydb)
    list(APPEND requirements_files
        "${USERVER_TESTSUITE_DIR}/requirements-ydb.txt")
  endif()

  if(USERVER_FEATURE_REDIS OR TARGET userver::redis)
    list(APPEND requirements_files
        "${USERVER_TESTSUITE_DIR}/requirements-redis.txt")
    list(APPEND testsuite_modules redis)
  endif()

  if(USERVER_FEATURE_CLICKHOUSE OR TARGET userver::clickhouse)
    list(APPEND testsuite_modules clickhouse)
  endif()

  if(USERVER_FEATURE_RABBITMQ OR TARGET userver::rabbitmq)
    list(APPEND testsuite_modules rabbitmq)
  endif()

  if(USERVER_FEATURE_MYSQL OR TARGET userver::mysql)
    list(APPEND testsuite_modules mysql)
  endif()

  file(READ "${USERVER_TESTSUITE_DIR}/requirements-testsuite.txt"
      requirements_testsuite_text)
  if(testsuite_modules)
    list(JOIN testsuite_modules "," testsuite_modules_str)
    string(
        REPLACE
        "yandex-taxi-testsuite[]"
        "yandex-taxi-testsuite[${testsuite_modules_str}]"
        requirements_testsuite_text
        "${requirements_testsuite_text}"
    )
  endif()
  set(requirements_testsuite_file
      "${CMAKE_BINARY_DIR}/requirements-userver-testsuite.txt")
  file(WRITE "${requirements_testsuite_file}" "${requirements_testsuite_text}")
  list(APPEND requirements_files "${requirements_testsuite_file}")

  set("${ARG_REQUIREMENT_FILES_VAR}" ${requirements_files} PARENT_SCOPE)
endfunction()

function(userver_testsuite_add)
  set(oneValueArgs
      SERVICE_TARGET
      WORKING_DIRECTORY
      PYTHON_BINARY
      PRETTY_LOGS
  )
  set(multiValueArgs
      PYTEST_ARGS
      REQUIREMENTS
      PYTHONPATH
      VIRTUALENV_ARGS
  )
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

  if(ARG_PYTHON_BINARY)
    if(ARG_REQUIREMENTS)
      message(FATAL_ERROR
          "PYTHON_BINARY and REQUIREMENTS options are incompatible")
    endif()
    set(python_binary "${ARG_PYTHON_BINARY}")
  else()
    userver_testsuite_requirements(REQUIREMENT_FILES_VAR requirement_files)
    list(APPEND requirement_files ${ARG_REQUIREMENTS})
    userver_venv_setup(
        NAME "${TESTSUITE_TARGET}"
        REQUIREMENTS ${requirement_files}
        PYTHON_OUTPUT_VAR python_binary
        VIRTUALENV_ARGS ${ARG_VIRTUALENV_ARGS}
    )
  endif()

  if(NOT python_binary)
    message(FATAL_ERROR "No python binary given.")
  endif()

  set(TESTSUITE_RUNNER "${CMAKE_CURRENT_BINARY_DIR}/runtests-${TESTSUITE_TARGET}")
  list(APPEND ARG_PYTHONPATH ${USERVER_TESTSUITE_DIR}/pytest_plugins)

  execute_process(
    COMMAND
    "${python_binary}" ${USERVER_TESTSUITE_DIR}/create_runner.py
    -o ${TESTSUITE_RUNNER}
    --python=${python_binary}
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
      NAME "${TESTSUITE_TARGET}"
      COMMAND
      "${TESTSUITE_RUNNER}"
      ${PRETTY_LOGS_MODE}
      -vv
      "${ARG_WORKING_DIRECTORY}"
  )

  add_custom_target(
      "start-${ARG_SERVICE_TARGET}"
      COMMAND
      "${TESTSUITE_RUNNER}"
      ${PRETTY_LOGS_MODE}
      --service-runner-mode
      -vvs
      "${ARG_WORKING_DIRECTORY}"
      DEPENDS
      "${TESTSUITE_RUNNER}"
      "${ARG_SERVICE_TARGET}"
      USES_TERMINAL
  )
endfunction()

# Tries to search service files in some standard places.
# Should be invoked from the service's CMakeLists.txt
# Supports the following file structure (and a few others):
# - configs/config.yaml
# - configs/config_vars.[testsuite|tests].yaml [optional]
# - configs/dynamic_config_fallback.json [optional]
# - configs/[secdist|secure_data].json [optional]
# - [testsuite|tests]/conftest.py
function(userver_testsuite_add_simple)
  set(oneValueArgs
      SERVICE_TARGET
      WORKING_DIRECTORY
      PYTHON_BINARY
      PRETTY_LOGS
      CONFIG_PATH
      CONFIG_VARS_PATH
      DYNAMIC_CONFIG_FALLBACK_PATH
      SECDIST_PATH
      TEST_ENV
  )
  set(multiValueArgs
      PYTEST_ARGS
      REQUIREMENTS
      PYTHONPATH
      VIRTUALENV_ARGS
  )
  cmake_parse_arguments(
      ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  set(pytest_additional_args)

  if(ARG_WORKING_DIRECTORY)
    if(IS_ABSOLUTE "${ARG_WORKING_DIRECTORY}")
      file(RELATIVE_PATH tests_relative_path
          "${CMAKE_CURRENT_SOURCE_DIR}" "${ARG_WORKING_DIRECTORY}")
    else()
      set(tests_relative_path "${ARG_WORKING_DIRECTORY}")
      get_filename_component(ARG_WORKING_DIRECTORY "${ARG_WORKING_DIRECTORY}"
          REALPATH BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    endif()
  else()
    foreach(probable_tests_path IN ITEMS
        "testsuite"
        "tests"
        "."
    )
      if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${probable_tests_path}/conftest.py")
        set(ARG_WORKING_DIRECTORY
            "${CMAKE_CURRENT_SOURCE_DIR}/${probable_tests_path}")
        set(tests_relative_path "${probable_tests_path}")
        break()
      endif()
    endforeach()
  endif()

  if(NOT ARG_SERVICE_TARGET)
    if(tests_relative_path STREQUAL "." OR tests_relative_path STREQUAL "tests")
      set(ARG_SERVICE_TARGET "${PROJECT_NAME}")
    else()
      set(ARG_SERVICE_TARGET "${PROJECT_NAME}-${tests_relative_path}")
    endif()
  endif()

  if(ARG_CONFIG_PATH)
    get_filename_component(config_path "${ARG_CONFIG_PATH}"
        REALPATH BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
  else()
    foreach(probable_config_path IN ITEMS
        "${CMAKE_CURRENT_SOURCE_DIR}/configs/static_config.yaml"
        "${CMAKE_CURRENT_SOURCE_DIR}/configs/config.yaml"
        "${CMAKE_CURRENT_SOURCE_DIR}/static_config.yaml"
        "${CMAKE_CURRENT_SOURCE_DIR}/config.yaml"
    )
      if(EXISTS "${probable_config_path}")
        set(config_path "${probable_config_path}")
        break()
      endif()
    endforeach()

    if(NOT config_path)
      message(FATAL_ERROR
          "Failed to find service static config for testsuite. "
          "Please pass it to ${CMAKE_CURRENT_FUNCTION} as CONFIG_PATH arg.")
    endif()
  endif()

  if(ARG_CONFIG_VARS_PATH)
    get_filename_component(config_vars_path "${ARG_CONFIG_VARS_PATH}"
        REALPATH BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
  else()
    foreach(probable_config_vars_path IN ITEMS
        "${CMAKE_CURRENT_SOURCE_DIR}/configs/config_vars.testsuite.yaml"
        "${CMAKE_CURRENT_SOURCE_DIR}/configs/config_vars.testing.yaml"
        "${CMAKE_CURRENT_SOURCE_DIR}/configs/config_vars.yaml"
        "${CMAKE_CURRENT_SOURCE_DIR}/config_vars.testsuite.yaml"
        "${CMAKE_CURRENT_SOURCE_DIR}/config_vars.testing.yaml"
        "${CMAKE_CURRENT_SOURCE_DIR}/config_vars.yaml"
    )
      if(EXISTS "${probable_config_vars_path}")
        set(config_vars_path "${probable_config_vars_path}")
        break()
      endif()
    endforeach()
  endif()
  if(config_vars_path)
    list(APPEND pytest_additional_args
        "--service-config-vars=${config_vars_path}")
  endif()

  if(ARG_DYNAMIC_CONFIG_FALLBACK_PATH)
    get_filename_component(dynamic_config_fallback_path
        "${ARG_DYNAMIC_CONFIG_FALLBACK_PATH}"
        REALPATH BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
  else()
    foreach(probable_dynamic_config_fallback_path IN ITEMS
        "${CMAKE_CURRENT_SOURCE_DIR}/configs/dynamic_config_fallback.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/dynamic_config_fallback.json"
    )
      if(EXISTS "${probable_dynamic_config_fallback_path}")
        set(dynamic_config_fallback_path
            "${probable_dynamic_config_fallback_path}")
        break()
      endif()
    endforeach()
  endif()
  if(dynamic_config_fallback_path)
    list(APPEND pytest_additional_args
        "--config-fallback=${dynamic_config_fallback_path}")
  endif()

  if(ARG_SECDIST_PATH)
    get_filename_component(secdist_path "${ARG_CONFIG_VARS_PATH}"
        REALPATH BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
  else()
    foreach(probable_secdist_path IN ITEMS
        "${CMAKE_CURRENT_SOURCE_DIR}/configs/secdist.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/configs/secure_data.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/secdist.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/secure_data.json"
    )
      if(EXISTS "${probable_secdist_path}")
        set(secdist_path "${probable_secdist_path}")
        break()
      endif()
    endforeach()
  endif()
  if(secdist_path)
    list(APPEND pytest_additional_args
        "--service-secdist=${secdist_path}")
  endif()

  if(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/proto")
    list(APPEND ARG_PYTHONPATH "${CMAKE_CURRENT_BINARY_DIR}/proto")
  endif()

  userver_testsuite_add(
      SERVICE_TARGET "${ARG_SERVICE_TARGET}"
      WORKING_DIRECTORY "${ARG_WORKING_DIRECTORY}"
      PYTHON_BINARY "${ARG_PYTHON_BINARY}"
      PRETTY_LOGS "${ARG_PRETTY_LOGS}"
      PYTEST_ARGS
      "--service-config=${config_path}"
      "--service-source-dir=${CMAKE_CURRENT_SOURCE_DIR}"
      "--service-binary=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}"
      ${pytest_additional_args}
      ${ARG_PYTEST_ARGS}
      REQUIREMENTS ${ARG_REQUIREMENTS}
      PYTHONPATH ${ARG_PYTHONPATH}
      VIRTUALENV_ARGS ${ARG_VIRTUALENV_ARGS}
  )

  if(ARG_TEST_ENV)
    set_tests_properties("testsuite-${ARG_SERVICE_TARGET}"
        PROPERTIES ENVIRONMENT ${ARG_TEST_ENV}
    )
  endif()
endfunction()
