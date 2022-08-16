include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-mcx16" HAS_mcx16)

if(HAS_mcx16)
  set(MCX16_ARG "-mcx16")
else()
  set(MCX16_ARG "")
endif()

try_run(
  RUN_RESULT COMPILE_RESULT
  "${CMAKE_CURRENT_BINARY_DIR}/require_dwcas"
  "${CMAKE_CURRENT_LIST_DIR}/RequireDWCAS.cpp"
  CMAKE_FLAGS "-DINCLUDE_DIRECTORIES=${Boost_INCLUDE_DIR}"
  COMPILE_DEFINITIONS "-std=c++17" ${MCX16_ARG}
  COMPILE_OUTPUT_VARIABLE COMPILE_OUTPUT
)

if(NOT "${COMPILE_RESULT}")
  message(SEND_ERROR "Error while compiling DWCAS test: ${COMPILE_OUTPUT}")
elseif(NOT "${RUN_RESULT}" STREQUAL "0")
  message(SEND_ERROR "We don't know how to make DWCAS work on this platform")
else()
  message(STATUS "DWCAS works")
endif()
