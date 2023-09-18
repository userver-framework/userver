if (TARGET CryptoPP)
    return()
endif()

option(
    USERVER_DOWNLOAD_PACKAGE_CRYPTOPP
    "Download and setup CryptoPP if no CryptoPP of matching version was found"
    ${USERVER_DOWNLOAD_PACKAGES}
)

if (NOT USERVER_FORCE_DOWNLOAD_PACKAGES)
  if (USERVER_DOWNLOAD_PACKAGE_CRYPTOPP)
      find_package(CryptoPP QUIET)
  else()
      find_package(CryptoPP REQUIRED)
  endif()

  if (CryptoPP_FOUND)
      return()
  endif()
endif()

include(DownloadUsingCPM)
# Letting cryptopp-cmake download cryptopp on its own results in:
#   cryptopp clone error
# Call Stack (most recent call first):
# cryptopp-cmake/cmake/GitClone.cmake:266 (message)
# cryptopp-cmake/cmake/GetCryptoppSources.cmake:18 (git_clone)
# cryptopp-cmake/cmake/GetCryptoppSources.cmake:60 (use_gitclone)
# cryptopp-cmake/CMakeLists.txt:172 (get_cryptopp_sources)
CPMAddPackage(
    NAME cryptopp
    VERSION 8.7.0
    GITHUB_REPOSITORY weidai11/cryptopp
    GIT_TAG CRYPTOPP_8_7_0
    DOWNLOAD_ONLY YES
)
CPMAddPackage(
    NAME cryptopp-cmake
    VERSION 8.7.0
    GITHUB_REPOSITORY abdes/cryptopp-cmake
    OPTIONS
    "CRYPTOPP_SOURCES ${cryptopp_SOURCE_DIR}"
    "cryptopp_DISPLAY_CMAKE_SUPPORT_WARNING OFF"
    "CRYPTOPP_BUILD_SHARED OFF"
    "CRYPTOPP_BUILD_TESTING OFF"
    "CRYPTOPP_USE_INTERMEDIATE_OBJECTS_TARGET OFF"
    "USE_OPENMP OFF"
    "CRYPTOPP_DATA_DIR"
)

add_library(CryptoPP INTERFACE)
target_link_libraries(CryptoPP INTERFACE cryptopp)
get_filename_component(cryptopp_parent_directory "${cryptopp_SOURCE_DIR}" DIRECTORY)
target_include_directories(CryptoPP INTERFACE "${cryptopp_parent_directory}")
target_compile_options(cryptopp PRIVATE "-Wno-implicit-fallthrough")
