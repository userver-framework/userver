# userver does not use Abseil directly, but some libraries need it.

option(USERVER_DOWNLOAD_PACKAGE_ABSEIL "Download and setup Abseil if no Abseil matching version was found" ${USERVER_DOWNLOAD_PACKAGES})
option(USERVER_FORCE_DOWNLOAD_ABSEIL "Download Abseil even if it exists in a system" ${USERVER_DOWNLOAD_PACKAGES})

if(NOT USERVER_FORCE_DOWNLOAD_ABSEIL)
  set(ABSL_PROPAGATE_CXX_STD ON)

  if(USERVER_DOWNLOAD_PACKAGE_ABSEIL)
    find_package(absl QUIET)
  else()
    find_package(absl REQUIRED)
  endif()

  if(absl_FOUND)
    return()
  endif()
endif()

include(DownloadUsingCPM)

CPMAddPackage(
    NAME abseil-cpp
    VERSION 20230802.1
    GIT_TAG 20230802.1
    GITHUB_REPOSITORY abseil/abseil-cpp
    SYSTEM
    OPTIONS
    "ABSL_PROPAGATE_CXX_STD ON"
    "ABSL_ENABLE_INSTALL ON"
)

mark_targets_as_system("${abseil-cpp_SOURCE_DIR}")
write_package_stub(absl)
