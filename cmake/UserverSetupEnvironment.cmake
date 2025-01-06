# Implementation note: public functions here should be usable even without
# a direct include of this script, so the functions should not rely
# on non-cache variables being present.
include_guard(GLOBAL)

set_property(GLOBAL PROPERTY userver_cmake_dir "${CMAKE_CURRENT_LIST_DIR}")

function(_userver_setup_environment_validate_impl)
  if(NOT USERVER_IMPL_SETUP_ENV_WAS_RUN_FOR_THIS_DIR)
    message(FATAL_ERROR
      "Looks like userver is included into the project as "
      "add_subdirectory(path/to/userver) or find_package(userver) in "
      "subdirectory of the project. In that case "
      "userver_setup_environment() should be called at the project root."
    )
    return()
  endif()
endfunction()

function(_userver_setup_environment_impl)
  if(USERVER_IMPL_SETUP_ENV_WAS_RUN_FOR_THIS_DIR)
    return()
  endif()
  set(USERVER_IMPL_SETUP_ENV_WAS_RUN_FOR_THIS_DIR ON PARENT_SCOPE)

  get_property(USERVER_CMAKE_DIR GLOBAL PROPERTY userver_cmake_dir)

  message(STATUS "C compiler: ${CMAKE_C_COMPILER}")
  message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")

  cmake_policy(SET CMP0057 NEW)
  if(NOT "${USERVER_CMAKE_DIR}/modules" IN_LIST CMAKE_MODULE_PATH AND NOT USERVER_CONAN)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${USERVER_CMAKE_DIR}/modules" PARENT_SCOPE)
  endif()
  if(NOT "${CMAKE_BINARY_DIR}/package_stubs" IN_LIST CMAKE_PREFIX_PATH)
    set(CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}/package_stubs" ${CMAKE_PREFIX_PATH} PARENT_SCOPE)
  endif()

  set(CMAKE_EXPORT_COMPILE_COMMANDS ON PARENT_SCOPE)
  if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD 17 PARENT_SCOPE)
  endif()
  message(STATUS "C++ standard ${CMAKE_CXX_STANDARD}")
  set(CMAKE_CXX_STANDARD_REQUIRED ON PARENT_SCOPE)
  set(CMAKE_CXX_EXTENSIONS OFF PARENT_SCOPE)
  set(CMAKE_VISIBILITY_INLINES_HIDDEN ON PARENT_SCOPE)

  add_compile_options("-pipe" "-g" "-gz" "-fPIC")
  add_compile_definitions("PIC=1")

  option(USERVER_COMPILATION_TIME_TRACE "Generate Clang compilation time trace" OFF)
  if(USERVER_COMPILATION_TIME_TRACE)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      message(FATAL_ERROR "USERVER_COMPILATION_TIME_TRACE is only supported for Clang")
    endif()
    add_compile_options("-ftime-trace")
  endif()

  include("${USERVER_CMAKE_DIR}/SetupLinker.cmake")
  include("${USERVER_CMAKE_DIR}/SetupLTO.cmake")
  include("${USERVER_CMAKE_DIR}/SetupPGO.cmake")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}" PARENT_SCOPE)
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}" PARENT_SCOPE)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION "${CMAKE_INTERPROCEDURAL_OPTIMIZATION}" PARENT_SCOPE)

  option(USERVER_USE_CCACHE "Use ccache for build" ON)
  if(USERVER_USE_CCACHE)
    find_program(CCACHE_EXECUTABLE ccache)
    if (CCACHE_EXECUTABLE)
      message(STATUS "ccache: enabled")
      if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.17)
        message(STATUS "Setting compiler launcher: ${CCACHE_EXECUTABLE}")
        set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_EXECUTABLE}" PARENT_SCOPE)
        set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_EXECUTABLE}" PARENT_SCOPE)
      else()
        message(STATUS "Setting rule launch compile: ${CCACHE_EXECUTABLE}")
        set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_EXECUTABLE}")
      endif()
    else()
      message(STATUS "ccache: enabled, but not found")
    endif()
  else()
    message(STATUS "ccache: disabled")
  endif()

  # Build type specific
  if (CMAKE_BUILD_TYPE MATCHES "Debug" OR CMAKE_BUILD_TYPE MATCHES "Test")
    add_compile_definitions(_GLIBCXX_ASSERTIONS)
    add_compile_definitions(BOOST_ENABLE_ASSERT_HANDLER)
  else()
    add_compile_definitions(NDEBUG)

    # enable additional glibc checks (used in debian packaging, requires -O)
    add_compile_definitions("_FORTIFY_SOURCE=2")
  endif()
endfunction()

macro(userver_setup_environment)
  _userver_setup_environment_impl()
  enable_testing()  # Does not work if placed into function
endmacro()
