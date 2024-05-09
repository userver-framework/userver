option(USERVER_DOWNLOAD_PACKAGE_PROTOBUF "Download and setup Protobuf" ${USERVER_DOWNLOAD_PACKAGE_GRPC})

function(_userver_get_protobuf_version_category OUT_VAR)
  if(Protobuf_VERSION VERSION_GREATER_EQUAL 5.26.0 AND
      Protobuf_VERSION VERSION_LESS 6.0.0 OR
      Protobuf_VERSION VERSION_GREATER_EQUAL 26.0.0)
    set("${OUT_VAR}" 5 PARENT_SCOPE)
  elseif(Protobuf_VERSION VERSION_GREATER_EQUAL 3.20.0 AND
      Protobuf_VERSION VERSION_LESS 4.0.0 OR
      Protobuf_VERSION VERSION_GREATER_EQUAL 4.20.0 AND
      Protobuf_VERSION VERSION_LESS 5.0.0 OR
      Protobuf_VERSION VERSION_GREATER_EQUAL 20.0.0)
    set("${OUT_VAR}" 4 PARENT_SCOPE)
  elseif(Protobuf_VERSION VERSION_GREATER 3.0.0 AND
      Protobuf_VERSION VERSION_LESS 4.0.0)
    set("${OUT_VAR}" 3 PARENT_SCOPE)
  else()
    message(FATAL_ERROR "Unsupported Protobuf_VERSION: ${Protobuf_VERSION}")
  endif()
endfunction()

if(USERVER_CONAN)
  find_package(Protobuf REQUIRED)
  _userver_get_protobuf_version_category(USERVER_PROTOBUF_VERSION_CATEGORY)
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
    _userver_get_protobuf_version_category(USERVER_PROTOBUF_VERSION_CATEGORY)
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
_userver_get_protobuf_version_category(USERVER_PROTOBUF_VERSION_CATEGORY)
