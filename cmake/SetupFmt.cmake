option(USERVER_FEATURE_FMT_DOWNLOAD "Download and setup Fmt if no Fmt of matching version was found" ${USERVER_FEATURE_DOWNLOAD_PACKAGES})
if (USERVER_FEATURE_FMT_DOWNLOAD)
    find_package(fmt "7.1")
else()
    find_package(fmt "7.1" REQUIRED)
endif()

if (fmt_FOUND)
    return()
endif()

include(FetchContent)
FetchContent_Declare(
  fmt_external_project
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  TIMEOUT 10
  GIT_TAG 8.1.1
  SOURCE_DIR ${USERVER_ROOT_DIR}/third_party/fmt
)

FetchContent_GetProperties(fmt_external_project)
if (NOT fmt_external_project_POPULATED)
  message(STATUS "Downloading Fmt from remote")
  FetchContent_Populate(fmt_external_project)
endif()

add_subdirectory(${USERVER_ROOT_DIR}/third_party/fmt "${CMAKE_BINARY_DIR}/third_party/fmt")
