# userver does not use Abseil directly, but some libraries need it.
#
# CPM package name must be "abseil-cpp" to match ydb-cpp-sdk and other
# dependencies; otherwise a second Abseil copy is added and clashes on absl_*
# targets. find_package still uses the conventional name "absl".

set(USERVER_ABSEIL_VERSION 20230802.1)

# @Note download
option(USERVER_DOWNLOAD_PACKAGE_ABSEIL "Download and setup Abseil if no Abseil matching version was found"
       ${USERVER_DOWNLOAD_PACKAGES}
)
# @Note download
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

cpmaddpackage(
    NAME abseil-cpp
    VERSION ${USERVER_ABSEIL_VERSION}
    URL https://github.com/abseil/abseil-cpp/archive/${USERVER_ABSEIL_VERSION}.tar.gz
    URL_HASH SHA256=987ce98f02eefbaf930d6e38ab16aa05737234d7afbab2d5c4ea7adbe50c28ed
    SYSTEM
    PATCHES abseil_pr_1707.patch abseil_pr_1739.patch
    OPTIONS "ABSL_PROPAGATE_CXX_STD ON" "ABSL_ENABLE_INSTALL OFF"
)

mark_targets_as_system("${abseil-cpp_SOURCE_DIR}")
write_package_stub(absl)
