if (TARGET spdlog_header_only)
    return()
endif()

option(
    USERVER_DOWNLOAD_PACKAGE_SPDLOG
    "Download and setup Spdlog if no Spdlog of matching version was found"
    ${USERVER_DOWNLOAD_PACKAGES}
)

if (NOT USERVER_FORCE_DOWNLOAD_PACKAGES)
  if (USERVER_DOWNLOAD_PACKAGE_SPDLOG)
    find_package(spdlog "1.9.0" QUIET)
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
endif()

include(SetupFmt)

include(DownloadUsingCPM)
CPMAddPackage(
    NAME spdlog
    VERSION 1.9.2
    GITHUB_REPOSITORY gabime/spdlog
    OPTIONS
    "SPDLOG_PREVENT_CHILD_FD ON"
    "SPDLOG_FMT_EXTERNAL ON"
)
