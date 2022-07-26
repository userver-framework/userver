if (USERVER_OPEN_SOURCE_BUILD)
  set(PYTHON_PACKAGE_NAME python3)
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  set(PYTHON ${Python3_EXECUTABLE})
else()
  # Clear caches
  unset(PYTHON CACHE)

  set(PYTHON_BINARY_NAME python3)
  set(PYTHON_PACKAGE_NAME "taxi-deps-py3-2" CACHE STRING
    "Python dependencies package name")
  set(PYTHON_ENV_PATH "/usr/lib/yandex/taxi-py3-2/bin" CACHE STRING
    "Path to python3 environment")

  # First of all try taxi-deps-py3
  find_program(PYTHON ${PYTHON_BINARY_NAME}
    PATHS ${PYTHON_ENV_PATH} NO_DEFAULT_PATH)

  if (${PYTHON} STREQUAL "PYTHON-NOTFOUND")
    if (NOT APPLE)
      message(WARNING
        "Python not found at ${PYTHON_ENV_PATH}. "
        "On Ubuntu please consider installing ${PYTHON_PACKAGE_NAME}:\n"
        "$ sudo apt-get install ${PYTHON_PACKAGE_NAME}\n"
        "'${PYTHON_PACKAGE_NAME}' not found, falling back to system "
        "${PYTHON_BINARY_NAME}")
    endif()
    find_program(PYTHON ${PYTHON_BINARY_NAME})
  endif()

  if (${PYTHON} STREQUAL "PYTHON-NOTFOUND")
    message(FATAL_ERROR
      "${PYTHON_BINARY_NAME} not found, try to install '${PYTHON_PACKAGE_NAME}'")
  endif()
endif()

message(STATUS "Python: ${PYTHON}")

function(python_requirements_check REQUIREMENTS_PATH)
  message(STATUS "Checking python requirements: ${REQUIREMENTS_PATH}")
  execute_process(
    COMMAND ${PYTHON} ${CMAKE_SOURCE_DIR}/scripts/check_requirements.py
    --deps ${PYTHON_PACKAGE_NAME}
    ${REQUIREMENTS_PATH}
    RESULT_VARIABLE RESULT
    OUTPUT_VARIABLE OUTPUT
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  if (RESULT)
    message(FATAL_ERROR "${OUTPUT}")
  endif(RESULT)
endfunction()
