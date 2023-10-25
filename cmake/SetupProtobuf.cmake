option(USERVER_DOWNLOAD_PACKAGE_PROTOBUF "Download and setup Protobuf" ${USERVER_DOWNLOAD_PACKAGE_GRPC})

if(USERVER_CONAN)
  find_package(Protobuf REQUIRED)
  return()
endif()

if(NOT USERVER_FORCE_DOWNLOAD_PACKAGES)
  # Use the builtin CMake FindProtobuf
  if(USERVER_DOWNLOAD_PACKAGE_PROTOBUF)
    find_package(Protobuf QUIET)
  else()
    find_package(Protobuf)
    if(NOT Protobuf_FOUND)
      message(FATAL_ERROR
          "userver failed to find Protobuf compiler.\n"
          "Please install the packages required for your system:\n\n"
          "  Debian:    sudo apt install protobuf-compiler python3-protobuf\n"
          "  macOS:     brew install protobuf\n"
          "  ArchLinux: sudo pacman -S protobuf\n"
          "  FreeBSD:   pkg install protobuf\n")
    endif()
  endif()

  if(Protobuf_FOUND)
    return()
  endif()
endif()

include(DownloadUsingCPM)
include(SetupAbseil)

CPMAddPackage(
    NAME Protobuf
    VERSION 4.24.4
    GITHUB_REPOSITORY protocolbuffers/protobuf
    SYSTEM
    OPTIONS
    "protobuf_BUILD_SHARED_LIBS OFF"
    "protobuf_BUILD_TESTS OFF"
    "protobuf_INSTALL OFF"
    "protobuf_MSVC_STATIC_RUNTIME OFF"
    "protobuf_ABSL_PROVIDER none"
)

set(Protobuf_VERSION "${CPM_PACKAGE_Protobuf_VERSION}")
set(Protobuf_FOUND TRUE)
set(PROTOBUF_INCLUDE_DIRS "${Protobuf_SOURCE_DIR}/src")
set(Protobuf_INCLUDE_DIR "${Protobuf_SOURCE_DIR}/src")
set_target_properties(libprotoc PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${Protobuf_SOURCE_DIR}/src")
write_package_stub(Protobuf)
mark_targets_as_system("${Protobuf_SOURCE_DIR}")
