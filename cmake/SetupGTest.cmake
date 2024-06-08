if(TARGET GTest::gtest)
  return()
endif()

option(USERVER_DOWNLOAD_PACKAGE_GTEST "Download and setup gtest if no gtest of matching version was found" ${USERVER_DOWNLOAD_PACKAGES})

if(NOT USERVER_FORCE_DOWNLOAD_PACKAGES)
  find_package(GTest QUIET)
  if(NOT GTest_FOUND AND NOT USERVER_DOWNLOAD_PACKAGE_GTEST)
    message(FATAL_ERROR
        "userver failed to find GTest package.\n"
        "Please install the packages required for your system:\n\n"
        "  Debian:    sudo apt install libgtest-dev libgmock-dev\n\n"
        "  macOS:     brew install googletest\n"
        "  ArchLinux: sudo pacman -S gtest\n"
        "  FreeBSD:   pkg install gtest\n")
  endif()

  if(GTest_FOUND)
    if(NOT TARGET GTest::gtest)
      add_library(GTest::gtest ALIAS GTest::GTest)
    endif()
    if(NOT TARGET GTest::gmock)
      add_library(GTest::gmock ALIAS GTest::GTest)
    endif()
    if(NOT TARGET GTest::gtest_main)
      add_library(GTest::gtest_main ALIAS GTest::Main)
    endif()
    if(NOT TARGET GTest::gmock_main)
      add_library(GTest::gmock_main ALIAS GTest::Main)
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

# Unify link names
if(NOT TARGET GTest::gtest)
  add_library(GTest::gtest ALIAS gtest)
  add_library(GTest::gmock ALIAS gmock)
  add_library(GTest::gtest_main ALIAS gtest_main)
  add_library(GTest::gmock_main ALIAS gmock_main)
endif()
