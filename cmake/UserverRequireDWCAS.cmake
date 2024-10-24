# Relies on variables from:
# - find_package(Boost)
# - userver_setup_environment
function (userver_target_require_dwcas target visibility)
  option(USERVER_FEATURE_DWCAS "Require double-width compare-exchange-swap" ON)

  if(NOT USERVER_FEATURE_DWCAS AND CMAKE_SYSTEM_NAME MATCHES "Darwin")
    message(FATAL_ERROR
        "All CPUs, which can run macOS, provide a DWCAS instruction. "
        "On the other hand, emulation via libatomic is not accessible there. "
        "Please remove -DUSERVER_FEATURE_DWCAS=OFF CMake option"
    )
  endif()

  if(NOT USERVER_FEATURE_DWCAS)
    message(STATUS "DWCAS disabled")
    return()
  endif()

  set(CMAKE_CXX_STANDARD 17)
  set(TEST_DEFINITIONS)
  set(TEST_LIBRARIES)

  if(CMAKE_SYSTEM_PROCESSOR MATCHES "^x86" OR CMAKE_SYSTEM_PROCESSOR MATCHES "^amd64")
    # Boost.Atomic 1.68+ produces correct DWCAS instructions on x86 and x86_64.
    set(BOOST_DWCAS_MIN_VERSION "1.68.0")
  else()
    # Boost.Atomic 1.74+ produces correct DWCAS instructions on ARM64.
    set(BOOST_DWCAS_MIN_VERSION "1.74.0")
  endif()

  # Prefer boost::atomic to std::atomic for two reasons.
  # Reason 1.
  # Clang's std::atomic already emits DWCAS instructions for x86,
  # x86_64 and armv8-a (a.k.a. ARM64) architectures (both libstdc++ and libc++).
  #
  # GCC's std::atomic falls back to libatomic:
  # https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80878
  # https://gcc.gnu.org/bugzilla/show_bug.cgi?id=84522
  #
  # Reason 2.
  # Clang still does not support std::atomic_ref, while there is
  # boost::atomic_ref and boost::atomic::value() since Boost 1.74.0.
  if("${Boost_VERSION_STRING}" VERSION_GREATER_EQUAL "${BOOST_DWCAS_MIN_VERSION}")
    message(STATUS "DWCAS: Using boost::atomic")
  else()
    message(WARNING "DWCAS: Using std::atomic")
    target_compile_definitions(${target} ${visibility} USERVER_USE_STD_DWCAS=1)
    list(APPEND TEST_DEFINITIONS "-DUSERVER_USE_STD_DWCAS=1")
  endif()

  set(BOOST_DWCAS_OPTIMAL_VERSION "1.74.0")
  if("${Boost_VERSION_STRING}" VERSION_LESS "${BOOST_DWCAS_OPTIMAL_VERSION}")
    message(
        WARNING
        "DWCAS: Performance may be suboptimal. "
        "Consider updating to Boost ${BOOST_DWCAS_OPTIMAL_VERSION}+."
    )
  endif()

  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang"
      AND (NOT USERVER_USE_LD MATCHES "lld" OR NOT CUSTOM_LD_OK)
      AND CMAKE_INTERPROCEDURAL_OPTIMIZATION)
    message(WARNING "DWCAS: Using a workaround to make sure it works with LTO "
        "under any linker. Performance may be sub-optimal. "
        "Define USERVER_USE_LD=lld to use the optimal linker.")
    target_compile_definitions(${target} ${visibility} USERVER_PROTECT_DWCAS=1)
  endif()

  include(CheckCXXCompilerFlag)
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    check_cxx_compiler_flag("-Wunused-command-line-argument -Werror -mcx16" HAS_mcx16)
  else()
    check_cxx_compiler_flag("-Werror -mcx16" HAS_mcx16)
  endif()

  if(HAS_mcx16)
    target_compile_options(${target} ${visibility} "-mcx16")
    list(APPEND TEST_DEFINITIONS "-mcx16")
  endif()

  if(USERVER_IMPL_DWCAS_CHECKED)
    message(STATUS "DWCAS works")
    return()
  endif()

  if(NOT CMAKE_SYSTEM_NAME MATCHES "Darwin" AND NOT CMAKE_SYSTEM MATCHES "BSD" AND NOT WIN32)
    list(APPEND TEST_LIBRARIES "atomic")
  endif()

  # Make try_run honor parent CMAKE_CXX_STANDARD
  cmake_policy(SET CMP0067 NEW)

  try_run(
      RUN_RESULT COMPILE_RESULT
      "${CMAKE_CURRENT_BINARY_DIR}/require_dwcas"
      "${USERVER_ROOT_DIR}/cmake/UserverRequireDWCAS.cpp"
      CMAKE_FLAGS "-DINCLUDE_DIRECTORIES=${Boost_INCLUDE_DIR}"
      COMPILE_DEFINITIONS ${TEST_DEFINITIONS}
      LINK_LIBRARIES ${TEST_LIBRARIES}
      COMPILE_OUTPUT_VARIABLE COMPILE_OUTPUT
  )

  if(NOT "${COMPILE_RESULT}")
    message(FATAL_ERROR "Error while compiling DWCAS test: ${COMPILE_OUTPUT}")
  elseif("${RUN_RESULT}" STREQUAL "1")
    message(WARNING "DWCAS seems not to work on the host platform, userver may have suboptimal performance. "
                    "Either use Clang compiler or install a modern C++ Boost Library (1.66 or hihgher for x86; Boost 1.74 or higher for ARM)"
    )
  elseif(NOT "${RUN_RESULT}" STREQUAL "0")
    message(WARNING "DWCAS does not work on the host platform, userver may crash with SIGILL")
  else()
    set(USERVER_IMPL_DWCAS_CHECKED "TRUE" CACHE INTERNAL "TRUE iff checked that DWCAS works")
    message(STATUS "DWCAS works - checked")
  endif()
endfunction()
