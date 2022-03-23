# AUTOGENERATED, DON'T CHANGE THIS FILE!

if (NOT mongoc_FIND_VERSION OR mongoc_FIND_VERSION VERSION_LESS 1.16.0)
    set(mongoc_FIND_VERSION 1.16.0)
endif()

if (NOT USERVER_CHECK_PACKAGE_VERSIONS)
  unset(mongoc_FIND_VERSION)
endif()

if (TARGET mongoc)
  if (NOT mongoc_FIND_VERSION)
      set(mongoc_FOUND ON)
      return()
  endif()

  if (mongoc_VERSION)
      if (mongoc_FIND_VERSION VERSION_LESS_EQUAL mongoc_VERSION)
          set(mongoc_FOUND ON)
          return()
      else()
          message(FATAL_ERROR
              "Already using version ${mongoc_VERSION} "
              "of mongoc when version ${mongoc_FIND_VERSION} "
              "was requested."
          )
      endif()
  endif()
endif()

set(FULL_ERROR_MESSAGE "Could not find `mongoc` package.\n\tDebian: sudo apt update && sudo apt install libmongoc-dev\n\tMacOS: brew install mongo-c-driver\n\tFedora: sudo dnf install mongo-c-driver-devel")


include(FindPackageHandleStandardArgs)

find_library(mongoc_LIBRARIES_mongoc
  NAMES mongoc
)
list(APPEND mongoc_LIBRARIES ${mongoc_LIBRARIES_mongoc})

find_path(mongoc_INCLUDE_DIRS_mongoc_mongoc_h
  NAMES mongoc/mongoc.h
  PATHS /usr/include/libmongoc-1.0
)
list(APPEND mongoc_INCLUDE_DIRS ${mongoc_INCLUDE_DIRS_mongoc_mongoc_h})



if (mongoc_VERSION)
  set(mongoc_VERSION ${mongoc_VERSION})
endif()

if (mongoc_FIND_VERSION AND NOT mongoc_VERSION)
  include(DetectVersion)

  if (UNIX AND NOT APPLE)
    deb_version(mongoc_VERSION libmongoc-dev)
    rpm_version(mongoc_VERSION mongo-c-driver-devel)
  endif()
  if (APPLE)
    brew_version(mongoc_VERSION mongo-c-driver)
  endif()
endif()

 
find_package_handle_standard_args(
  mongoc
    REQUIRED_VARS
      mongoc_LIBRARIES
      mongoc_INCLUDE_DIRS
      
    FAIL_MESSAGE
      "${FULL_ERROR_MESSAGE}"
)
mark_as_advanced(
  mongoc_LIBRARIES
  mongoc_INCLUDE_DIRS
  
)

if (NOT mongoc_FOUND)
  if (mongoc_FIND_REQUIRED)
      message(FATAL_ERROR "${FULL_ERROR_MESSAGE}. Required version is at least ${mongoc_FIND_VERSION}")
  endif()

  return()
endif()

if (mongoc_FIND_VERSION)
  if (mongoc_VERSION VERSION_LESS mongoc_FIND_VERSION)
      message(STATUS
          "Version of mongoc is '${mongoc_VERSION}'. "
          "Required version is at least '${mongoc_FIND_VERSION}'. "
          "Ignoring found mongoc."
          "Note: Set -DUSERVER_CHECK_PACKAGE_VERSIONS=0 to skip package version checks if the package is fine."
      )
      set(mongoc_FOUND OFF)
      return()
  endif()
endif()

 
if (NOT TARGET mongoc)
  add_library(mongoc INTERFACE IMPORTED GLOBAL)

  target_include_directories(mongoc INTERFACE ${mongoc_INCLUDE_DIRS})
  target_link_libraries(mongoc INTERFACE ${mongoc_LIBRARIES})
  
  # Target mongoc is created
endif()

if (mongoc_VERSION)
  set(mongoc_VERSION "${mongoc_VERSION}" CACHE STRING "Version of the mongoc")
endif()
