option(USERVER_FEATURE_GTEST_DOWNLOAD "Download and setup gtest if no gtest of matching version was found" ${USERVER_FEATURE_DOWNLOAD_PACKAGES})
if (USERVER_FEATURE_GTEST_DOWNLOAD)
    find_package(UserverGTest)
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

include(FetchContent)
FetchContent_Declare(
  gtest_external_project
  GIT_REPOSITORY https://github.com/google/googletest.git
  TIMEOUT 10
  GIT_TAG release-1.11.0
  SOURCE_DIR ${USERVER_ROOT_DIR}/third_party/gtest
)

FetchContent_GetProperties(gtest_external_project)
if (NOT gtest_external_project_POPULATED)
  message(STATUS "Downloading gtest from remote")
  FetchContent_Populate(gtest_external_project)
endif()

set(INSTALL_GTEST OFF CACHE BOOL "")
add_subdirectory(${USERVER_ROOT_DIR}/third_party/gtest "${CMAKE_BINARY_DIR}/third_party/gtest")
add_library(libgtest ALIAS gtest_main)  # Unify link names
add_library(libgmock ALIAS gmock)  # Unify link names

add_library(UserverGTest ALIAS gtest)
set(UserverGTest_VERSION "1.10.0" CACHE STRING "Version of the gtest")
