set(USERVER_PYTHON_PATH "python3" CACHE FILEPATH "Path to python3 executable to use")
set(PYTHON "${USERVER_PYTHON_PATH}")

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
