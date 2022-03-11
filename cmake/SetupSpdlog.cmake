if (NOT ${OPEN_SOURCE_BUILD})
  find_package(Helperspdlog REQUIRED)
  add_library(spdlog_header_only ALIAS spdlog)  # Unify link names
  return()
endif()

option(USERVER_FEATURE_SPDLOG_DOWNLOAD "Download and setup Spdlog if no Spdlog of matching version was found" ${USERVER_DOWNLOAD_PACKAGES})
if (USERVER_FEATURE_SPDLOG_DOWNLOAD)
    find_package(spdlog "1.6.0")
else()
    find_package(spdlog "1.6.0" REQUIRED)
endif()

if (spdlog_FOUND)
    if (NOT TARGET spdlog_header_only)
      if (TARGET spdlog)
          add_library(spdlog_header_only ALIAS spdlog)  # Unify link names
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
FetchContent_GetProperties(spdlog_external_project)
if(NOT spdlog_external_project_POPULATED)
  message(STATUS "Downloading spdlog from remote")
  FetchContent_Populate(spdlog_external_project)
endif()

include(SetupFmt)

set(SPDLOG_FMT_EXTERNAL ON CACHE BOOL "")
add_subdirectory(${USERVER_ROOT_DIR}/third_party/spdlog "${CMAKE_BINARY_DIR}/third_party/spdlog")
set(spdlog_VERSION "1.9.2" CACHE STRING "Version of the spdlog")
