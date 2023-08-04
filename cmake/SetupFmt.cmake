option(USERVER_DOWNLOAD_PACKAGE_FMT "Download and setup Fmt if no Fmt of matching version was found" ${USERVER_DOWNLOAD_PACKAGES})

if (NOT USERVER_FORCE_DOWNLOAD_PACKAGES)
  if (USERVER_DOWNLOAD_PACKAGE_FMT)
    find_package(fmt "8.1.1" QUIET)
  else()
    find_package(fmt "8.1.1" REQUIRED)
  endif()

  if (fmt_FOUND)
      return()
  endif()
endif()

include(DownloadUsingCPM)
CPMAddPackage(
    NAME fmt
    GIT_TAG 8.1.1
    GITHUB_REPOSITORY fmtlib/fmt
)
