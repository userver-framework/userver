if (TARGET spdlog_header_only)
    return()
endif()

macro(userver_add_spdlog_subdirectory SPDLOG_PATH_SUFFIX)
  include(SetupFmt)

  set(SPDLOG_PREVENT_CHILD_FD ON CACHE BOOL "")
  set(SPDLOG_FMT_EXTERNAL ON CACHE BOOL "")
  add_subdirectory("${USERVER_ROOT_DIR}/${SPDLOG_PATH_SUFFIX}" "${CMAKE_BINARY_DIR}/${SPDLOG_PATH_SUFFIX}")
  set(spdlog_VERSION "1.9.2" CACHE STRING "Version of the spdlog")
endmacro()

macro(userver_fetch_and_add_spdlog_subdirectory)
  FetchContent_GetProperties(spdlog_external_project)
  if(NOT spdlog_external_project_POPULATED)
    message(STATUS "Downloading spdlog from remote")
    FetchContent_Populate(spdlog_external_project)
  endif()

  userver_add_spdlog_subdirectory("third_party/spdlog")
endmacro()

option(USERVER_DOWNLOAD_PACKAGE_SPDLOG "Download and setup Spdlog if no Spdlog of matching version was found" ${USERVER_DOWNLOAD_PACKAGES})
if (USERVER_DOWNLOAD_PACKAGE_SPDLOG)
  find_package(spdlog "1.9.0")
else()
  find_package(spdlog "1.9.0" REQUIRED)
endif()

if (spdlog_FOUND)
    if (NOT TARGET spdlog_header_only)
      if (TARGET spdlog)
          add_library(spdlog_header_only ALIAS spdlog)  # Unify link names
      elseif(TARGET spdlog::spdlog_header_only)
          add_library(spdlog_header_only ALIAS spdlog::spdlog_header_only)  # Unify link names
      else()
          add_library(spdlog_header_only ALIAS spdlog::spdlog)  # Unify link names
      endif()
    endif()
    return()
endif()

# We fetch spdlog and add it with add_subdirectory() to force it to use our fmt

include(FetchContent)
FetchContent_Declare(
  spdlog_external_project
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  TIMEOUT 10
  GIT_TAG v1.9.2
  SOURCE_DIR ${USERVER_ROOT_DIR}/third_party/spdlog
)
userver_fetch_and_add_spdlog_subdirectory()
