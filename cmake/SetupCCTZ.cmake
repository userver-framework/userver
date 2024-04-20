option(USERVER_DOWNLOAD_PACKAGE_CCTZ "Download and setup cctz if no cctz of matching version was found" ${USERVER_DOWNLOAD_PACKAGES})

if (NOT USERVER_FORCE_DOWNLOAD_PACKAGES)
    if (USERVER_DOWNLOAD_PACKAGE_CCTZ)
        find_package(cctz QUIET)
    else()
        find_package(cctz REQUIRED)
    endif()

    if (cctz_FOUND)
        return()
    endif()
endif()

include(DownloadUsingCPM)
CPMAddPackage(
    NAME cctz
    VERSION 2.3
    GITHUB_REPOSITORY google/cctz
    OPTIONS
    "BUILD_TOOLS OFF"
    "BUILD_EXAMPLES OFF"
    "BUILD_TESTING OFF"
)
_userver_install_targets(COMPONENT universal TARGETS cctz)
