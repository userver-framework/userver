if(TARGET aklomp::base64)
    return()
endif()

# @ingroup download
option(USERVER_DOWNLOAD_PACKAGE_BASE64 "Download and setup aklomp/base64 if no aklomp/base64 of matching version was found"
       ${USERVER_DOWNLOAD_PACKAGES}
)

if(NOT USERVER_FORCE_DOWNLOAD_PACKAGES)
    if(USERVER_DOWNLOAD_PACKAGE_BASE64)
        find_package(base64 0.5.2 QUIET CONFIG)
    else()
        find_package(base64 0.5.2 REQUIRED CONFIG)
    endif()

    if(base64_FOUND)
        return()
    endif()
endif()

include(DownloadUsingCPM)
cpmaddpackage(
    NAME base64
    VERSION 0.5.2
    URL https://github.com/aklomp/base64/archive/v0.5.2.tar.gz
    URL_HASH SHA256=723a0f9f4cf44cf79e97bcc315ec8f85e52eb104c8882942c3f2fba95acc080d
    OPTIONS "CMAKE_SKIP_INSTALL_RULES ON" "BASE64_BUILD_CLI OFF" "BASE64_BUILD_TESTS OFF" "BASE64_WITH_OpenMP OFF"
            "BASE64_WERROR OFF"
)
write_package_stub(base64)
add_library(aklomp::base64 ALIAS base64)
_userver_install_targets(COMPONENT universal TARGETS base64)
