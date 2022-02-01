# AUTOGENERATED, DON'T CHANGE THIS FILE!

if (NOT cctz_FIND_VERSION OR cctz_FIND_VERSION VERSION_LESS 2.3)
    set(cctz_FIND_VERSION 2.3)
endif()

if (TARGET cctz)
  if (NOT cctz_FIND_VERSION)
      set(cctz_FOUND ON)
      return()
  endif()

  if (cctz_VERSION)
      if (cctz_FIND_VERSION VERSION_LESS_EQUAL cctz_VERSION)
          set(cctz_FOUND ON)
          return()
      else()
          message(FATAL_ERROR
              "Already using version ${cctz_VERSION} "
              "of cctz when version ${cctz_FIND_VERSION} "
              "was requested."
          )
      endif()
  endif()
endif()

set(FULL_ERROR_MESSAGE "Could not find `cctz` package.\n\tDebian: sudo apt update && sudo apt install libcctz-dev\n\tMacOS: brew install cctz-dev")


include(FindPackageHandleStandardArgs)

find_library(cctz_LIBRARIES_cctz
  NAMES cctz
  PATHS /usr/lib/x86_64-linux-gnu
)
list(APPEND cctz_LIBRARIES ${cctz_LIBRARIES_cctz})

find_path(cctz_INCLUDE_DIRS_cctz_civil_time_h
  NAMES cctz/civil_time.h
)
list(APPEND cctz_INCLUDE_DIRS ${cctz_INCLUDE_DIRS_cctz_civil_time_h})



if (cctz_VERSION)
  set(cctz_VERSION ${cctz_VERSION})
endif()

if (cctz_FIND_VERSION AND NOT cctz_VERSION)
  include(DetectVersion)

  if (UNIX AND NOT APPLE)
    deb_version(cctz_VERSION libcctz-dev)
  endif()
  if (APPLE)
    brew_version(cctz_VERSION cctz-dev)
  endif()
endif()

 
find_package_handle_standard_args(
  cctz
    REQUIRED_VARS
      cctz_LIBRARIES
      cctz_INCLUDE_DIRS
      
    FAIL_MESSAGE
      "${FULL_ERROR_MESSAGE}"
)
mark_as_advanced(
  cctz_LIBRARIES
  cctz_INCLUDE_DIRS
  
)

if (NOT cctz_FOUND)
  if (cctz_FIND_REQUIRED)
      message(FATAL_ERROR "${FULL_ERROR_MESSAGE}. Required version is at least ${cctz_FIND_VERSION}")
  endif()

  return()
endif()

if (cctz_FIND_VERSION)
  if (cctz_VERSION VERSION_LESS cctz_FIND_VERSION)
      message(STATUS
          "Version of cctz is '${cctz_VERSION}'. "
          "Required version is at least '${cctz_FIND_VERSION}'. "
          "Ignoring found cctz."
      )
      set(cctz_FOUND OFF)
      return()
  endif()
endif()

 
if (NOT TARGET cctz)
  add_library(cctz INTERFACE IMPORTED GLOBAL)

  target_include_directories(cctz INTERFACE ${cctz_INCLUDE_DIRS})
  target_link_libraries(cctz INTERFACE ${cctz_LIBRARIES})
  
  # Target cctz is created
endif()

if (cctz_VERSION)
  set(cctz_VERSION "${cctz_VERSION}" CACHE STRING "Version of the cctz")
endif()
