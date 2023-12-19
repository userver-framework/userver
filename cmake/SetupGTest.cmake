if (TARGET libgtest)
    return()
endif()

option(USERVER_DOWNLOAD_PACKAGE_GTEST "Download and setup gtest if no gtest of matching version was found" ${USERVER_DOWNLOAD_PACKAGES})

if (NOT USERVER_FORCE_DOWNLOAD_PACKAGES)
    if (USERVER_DOWNLOAD_PACKAGE_GTEST)
        find_package(UserverGTest QUIET)
    else()
        find_package(UserverGTest REQUIRED)
    endif()

    if (UserverGTest_FOUND)
        if (NOT TARGET libgtest)
            add_library(libgtest ALIAS UserverGTest)  # Unify link names
            add_library(libgmock ALIAS UserverGTest)  # Unify link names
        endif()
        return()
    endif()
endif()

include(DownloadUsingCPM)
CPMAddPackage(
    NAME googletest
    VERSION 1.14.0
    GIT_TAG v1.14.0
    GITHUB_REPOSITORY google/googletest
    OPTIONS "INSTALL_GTEST OFF"
)

add_library(libgtest ALIAS gtest_main)  # Unify link names
add_library(libgmock ALIAS gmock)  # Unify link names
