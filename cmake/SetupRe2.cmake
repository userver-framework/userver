option(USERVER_DOWNLOAD_PACKAGE_RE2 "Download and setup Re2 if no Re2 matching version was found"
       ${USERVER_DOWNLOAD_PACKAGES}
)
option(USERVER_FORCE_DOWNLOAD_RE2 "Download Re2 even if it exists in a system" ${USERVER_FORCE_DOWNLOAD_PACKAGES})

if(NOT USERVER_FORCE_DOWNLOAD_RE2)
    if(USERVER_DOWNLOAD_PACKAGE_RE2)
        find_package(re2 QUIET)
    else()
        find_package(re2 REQUIRED)
    endif()

    if(re2_FOUND)
        return()
    endif()
endif()

include(DownloadUsingCPM)

cpmaddpackage(
    NAME
    re2
    VERSION
    2023-03-01 # newest version without abseil requirements
    GIT_TAG
    2023-03-01
    GITHUB_REPOSITORY
    google/re2
    SYSTEM
    OPTIONS
    "RE2_BUILD_TESTING OFF"
    "RE2_USE_ICU ON"
)

mark_targets_as_system("${re2_SOURCE_DIR}")
write_package_stub(re2)
