if (TARGET CryptoPP)
    return()
endif()

option(USERVER_DOWNLOAD_PACKAGE_CRYPTOPP "Download and setup CryptoPP if no CryptoPP of matching version was found" ${USERVER_DOWNLOAD_PACKAGES})
if (USERVER_DOWNLOAD_PACKAGE_CRYPTOPP)
    find_package(CryptoPP)
else()
    find_package(CryptoPP REQUIRED)
endif()

if (CryptoPP_FOUND)
    return()
endif()

include(FetchContent)
FetchContent_Declare(
  cryptopp_external_project
  GIT_REPOSITORY https://github.com/weidai11/cryptopp.git
  TIMEOUT 10
  GIT_TAG CRYPTOPP_8_6_0
  SOURCE_DIR ${USERVER_ROOT_DIR}/third_party/cryptopp
)

FetchContent_GetProperties(cryptopp_external_project)
if (NOT cryptopp_external_project_POPULATED)
  message(STATUS "Downloading CryptoPP from remote")
  FetchContent_Populate(cryptopp_external_project)

  file(DOWNLOAD
	https://raw.githubusercontent.com/noloader/cryptopp-cmake/CRYPTOPP_8_6_0/CMakeLists.txt
    ${USERVER_ROOT_DIR}/third_party/cryptopp/CMakeLists.txt
  )
  file(DOWNLOAD
	https://raw.githubusercontent.com/noloader/cryptopp-cmake/CRYPTOPP_8_6_0/cryptopp-config.cmake
    ${USERVER_ROOT_DIR}/third_party/cryptopp/cryptopp-config.cmake
  )
endif()

set(cryptopp_DISPLAY_CMAKE_SUPPORT_WARNING OFF CACHE BOOL "")
set(BUILD_SHARED OFF CACHE BOOL "")
set(BUILD_TESTING OFF CACHE BOOL "")
set(USE_INTERMEDIATE_OBJECTS_TARGET OFF CACHE BOOL "")
set(USE_OPENMP OFF CACHE BOOL "")
set(CRYPTOPP_DATA_DIR "" CACHE STRING "")
add_subdirectory(${USERVER_ROOT_DIR}/third_party/cryptopp "${CMAKE_BINARY_DIR}/third_party/cryptopp")
set(CryptoPP_VERSION "8.6.0" CACHE STRING "Version of the CryptoPP")
add_library(CryptoPP INTERFACE)
target_link_libraries(CryptoPP INTERFACE cryptopp-static)
target_include_directories(CryptoPP INTERFACE "${USERVER_ROOT_DIR}/third_party/")
