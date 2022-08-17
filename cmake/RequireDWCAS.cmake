option(
  USERVER_FEATURE_DWCAS
  "Require double-word compare-exchange-swap instruction on the target platform"
  ON
)

if(NOT USERVER_FEATURE_DWCAS)
  add_compile_definitions(BOOST_ATOMIC_NO_CMPXCHG16B=1)
  message(STATUS "DWCAS disabled")
  return()
endif()

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-mcx16" HAS_mcx16)
if(HAS_mcx16)
  add_compile_options("-mcx16")
endif()

if(USERVER_IMPL_DWCAS_CHECKED)
  message(STATUS "DWCAS works")
  return()
endif()

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
  message(WARNING "DWCAS does not work on the host platform, userver may crash with SIGILL")
else()
  set(USERVER_IMPL_DWCAS_CHECKED "TRUE" CACHE INTERNAL "TRUE iff checked that DWCAS works")
  message(STATUS "DWCAS works - checked")
endif()
