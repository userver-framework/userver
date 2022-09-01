option(USERVER_FEATURE_DWCAS "Require double-width compare-exchange-swap" ON)

if(MACOS AND NOT USERVER_FEATURE_DWCAS)
  message(WARNING "macOS must use DWCAS, because atomic runtime is absent.")
  set(USERVER_FEATURE_DWCAS ON)
endif()

if(NOT USERVER_FEATURE_DWCAS)
  message(STATUS "DWCAS disabled")
  return()
endif()

add_compile_definitions(USERVER_FEATURE_DWCAS=1)

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

set(BOOST_CMAKE_VERSION "${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}.${Boost_SUBMINOR_VERSION}")

if(NOT MACOS)
  set(TEST_LIBRARIES atomic)
else()
  set(TEST_LIBRARIES)
endif()

# Make try_run honor parent CMAKE_CXX_STANDARD
cmake_policy(SET CMP0067 NEW)

try_run(
  RUN_RESULT COMPILE_RESULT
  "${CMAKE_CURRENT_BINARY_DIR}/require_dwcas"
  "${USERVER_ROOT_DIR}/cmake/RequireDWCAS.cpp"
  CMAKE_FLAGS "-DINCLUDE_DIRECTORIES=${Boost_INCLUDE_DIR}"
  COMPILE_DEFINITIONS ${MCX16_ARG}
  LINK_LIBRARIES ${TEST_LIBRARIES}
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
