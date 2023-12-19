option(USERVER_DOWNLOAD_PACKAGE_CARES "Download and setup c-ares if no c-ares of matching version was found" ${USERVER_DOWNLOAD_PACKAGES})

if (NOT USERVER_FORCE_DOWNLOAD_PACKAGES)
  if (USERVER_DOWNLOAD_PACKAGE_CARES)
    find_package(c-ares 1.16 QUIET)
  else()
    find_package(c-ares 1.16 REQUIRED)
  endif()

  if (c-ares_FOUND)
    if(NOT TARGET c-ares::cares)
      add_library(c-ares::cares ALIAS c-ares)
    endif()
    return()
  endif()
endif()

include(DownloadUsingCPM)
CPMAddPackage(
    NAME c-ares
    VERSION 1.19.0
    GITHUB_REPOSITORY c-ares/c-ares
    GIT_TAG cares-1_19_0
    OPTIONS
    "CARES_INSTALL OFF"
    "CARES_BUILD_TOOLS OFF"
    "CARES_SHARED OFF"
    "CARES_STATIC ON"
)

set(c-ares_FOUND TRUE)
write_package_stub(c-ares)
