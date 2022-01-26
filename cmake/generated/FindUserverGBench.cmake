# AUTOGENERATED, DON'T CHANGE THIS FILE!


if (TARGET UserverGBench)
  if (NOT UserverGBench_FIND_VERSION)
      set(UserverGBench_FOUND ON)
      return()
  endif()

  if (UserverGBench_VERSION)
      if (UserverGBench_FIND_VERSION VERSION_LESS_EQUAL UserverGBench_VERSION)
          set(UserverGBench_FOUND ON)
          return()
      else()
          message(FATAL_ERROR
              "Already using version ${UserverGBench_VERSION} "
              "of UserverGBench when version ${UserverGBench_FIND_VERSION} "
              "was requested."
          )
      endif()
  endif()
endif()

set(FULL_ERROR_MESSAGE "Could not find `UserverGBench` package.\n\tDebian: sudo apt update && sudo apt install libbenchmark-dev\n\tMacOS: brew install libbenchmark-dev")


include(FindPackageHandleStandardArgs)

find_library(UserverGBench_LIBRARIES_benchmark_main
  NAMES benchmark_main
)
list(APPEND UserverGBench_LIBRARIES ${UserverGBench_LIBRARIES_benchmark_main})

find_library(UserverGBench_LIBRARIES_benchmark
  NAMES benchmark
)
list(APPEND UserverGBench_LIBRARIES ${UserverGBench_LIBRARIES_benchmark})

find_path(UserverGBench_INCLUDE_DIRS_benchmark_benchmark_h
  NAMES benchmark/benchmark.h
)
list(APPEND UserverGBench_INCLUDE_DIRS ${UserverGBench_INCLUDE_DIRS_benchmark_benchmark_h})



if (UserverGBench_VERSION)
  set(UserverGBench_VERSION ${UserverGBench_VERSION})
endif()

if (UserverGBench_FIND_VERSION AND NOT UserverGBench_VERSION)
if (UNIX AND NOT APPLE)
  find_program(DPKG_QUERY_BIN dpkg-query)
  if (DPKG_QUERY_BIN)
    execute_process(
      COMMAND dpkg-query --showformat=\${Version} --show libbenchmark-dev
      OUTPUT_VARIABLE UserverGBench_version_output
      ERROR_VARIABLE UserverGBench_version_error
      RESULT_VARIABLE UserverGBench_version_result
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if (UserverGBench_version_result EQUAL 0)
      set(UserverGBench_VERSION ${UserverGBench_version_output})
      message(STATUS "Installed version libbenchmark-dev: ${UserverGBench_VERSION}")
    endif(UserverGBench_version_result EQUAL 0)
  endif(DPKG_QUERY_BIN)
endif(UNIX AND NOT APPLE)
 
if (APPLE)
  find_program(BREW_BIN brew)
  if (BREW_BIN)
    execute_process(
      COMMAND brew list --versions libbenchmark-dev
      OUTPUT_VARIABLE UserverGBench_version_output
      ERROR_VARIABLE UserverGBench_version_error
      RESULT_VARIABLE UserverGBench_version_result
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if (UserverGBench_version_result EQUAL 0)
      if (UserverGBench_version_output MATCHES "^(.*) (.*)$")
        set(UserverGBench_VERSION ${CMAKE_MATCH_2})
        message(STATUS "Installed version libbenchmark-dev: ${UserverGBench_VERSION}")
      else()
        set(UserverGBench_VERSION "NOT_FOUND")
      endif()
    else()
      message(WARNING "Failed execute brew: ${UserverGBench_version_error}")
    endif()
  endif()
endif()
 
endif (UserverGBench_FIND_VERSION AND NOT UserverGBench_VERSION)

 
find_package_handle_standard_args(
  UserverGBench
    REQUIRED_VARS
      UserverGBench_LIBRARIES
      UserverGBench_INCLUDE_DIRS
      
    FAIL_MESSAGE
      "${FULL_ERROR_MESSAGE}"
)
mark_as_advanced(
  UserverGBench_LIBRARIES
  UserverGBench_INCLUDE_DIRS
  
)

if (NOT UserverGBench_FOUND)
  if (UserverGBench_FIND_REQUIRED)
      message(FATAL_ERROR "${FULL_ERROR_MESSAGE}. Required version is at least ${UserverGBench_FIND_VERSION}")
  endif()

  return()
endif()

if (UserverGBench_FIND_VERSION)
  if (UserverGBench_VERSION VERSION_LESS UserverGBench_FIND_VERSION)
      message(STATUS
          "Version of UserverGBench is '${UserverGBench_VERSION}'. "
          "Required version is at least '${UserverGBench_FIND_VERSION}'. "
          "Ignoring found UserverGBench."
      )
      set(UserverGBench_FOUND OFF)
      return()
  endif()
endif()

 
if (NOT TARGET UserverGBench)
  add_library(UserverGBench INTERFACE IMPORTED GLOBAL)

  target_include_directories(UserverGBench INTERFACE ${UserverGBench_INCLUDE_DIRS})
  target_link_libraries(UserverGBench INTERFACE ${UserverGBench_LIBRARIES})
  
  # Target UserverGBench is created
endif()

if (UserverGBench_VERSION)
  set(UserverGBench_VERSION "${UserverGBench_VERSION}" CACHE STRING "Version of the UserverGBench")
endif()
