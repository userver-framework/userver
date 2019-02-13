set(TESTSUITE_PYTHONPATH ${CMAKE_CURRENT_SOURCE_DIR}/testsuite)
if (NOT PYTHON)
  set(PYTHON python3)
endif()
set(TESTSUITE_PYTHON_BINARY ${PYTHON})

configure_file(taxi-env.in taxi-env @ONLY)
