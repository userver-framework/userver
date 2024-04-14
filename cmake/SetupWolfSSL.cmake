if (TARGET wolfssl)
    return()
endif()

option(
    USERVER_DOWNLOAD_PACKAGE_WOLFSSL
    "Download and setup WolfSSL if no WolfSSL of matching version was found"
    ${USERVER_DOWNLOAD_PACKAGES}
)

if (NOT USERVER_FORCE_DOWNLOAD_PACKAGES)
  if (USERVER_DOWNLOAD_PACKAGE_WOLFSSL)
      find_package(wolfssl QUIET)
  else()
      find_package(wolfssl REQUIRED)
  endif()

  if (wolfssl_FOUND)
      return()
  endif()
endif()

include(DownloadUsingCPM)

find_package(Patch REQUIRED)
message(STATUS "${wolfssl_parent_directory}")
message(STATUS "${CMAKE_CURRENT_LIST_DIR}")
CPMAddPackage(
    NAME WolfSSL
    VERSION 5.7.0
    GITHUB_REPOSITORY wolfSSL/wolfssl
    GIT_TAG v5.7.0-stable
    PATCH_COMMAND
        "${Patch_EXECUTABLE}" --merge -p1 < "${CMAKE_CURRENT_LIST_DIR}/patches/wolfssl-0001-build-fixes.patch"
    OPTIONS
    "BUILD_SHARED_LIBS OFF"
    "WOLFSSL_BUILD_TESTING OFF"
    "CMAKE_C_FLAGS -Wall -Wextra -O2 -DOPENSSL_ALL -DOPENSSL_EXTRA"
)

#add_library(WolfSSL INTERFACE)
#target_link_libraries(WolfSSL INTERFACE wolfssl)
#get_filename_component(wolfssl_parent_directory "${WolfSSL_SOURCE_DIR}" DIRECTORY)
#target_include_directories(WolfSSL INTERFACE "${wolfssl_parent_directory}/wolfssl")
#target_compile_options(WolfSSL PRIVATE "-O2")
