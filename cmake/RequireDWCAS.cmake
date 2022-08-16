include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-mcx16" HAS_MCX16)

if(HAS_MCX16)
  set(MCX16_ARG "-mcx16")
else()
  set(MCX16_ARG "")
endif()

try_run(
  RUN_RESULT COMPILE_RESULT
  "${CMAKE_CURRENT_BINARY_DIR}/require_dwcas"
  "${CMAKE_SOURCE_DIR}/cmake/RequireDWCAS.cpp"
  CMAKE_FLAGS "-DINCLUDE_DIRECTORIES=${Boost_INCLUDE_DIR}"
  COMPILE_DEFINITIONS "-std=c++17" ${MCX16_ARG}
)

if(NOT "${RUN_RESULT}" STREQUAL "0")
  message(SEND_ERROR "We don't know how to make DWCAS work on this platform")
endif()
