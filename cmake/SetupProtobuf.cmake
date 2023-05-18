if (USERVER_CONAN)
  find_package(Protobuf REQUIRED)
else()
  # Use the builtin CMake FindProtobuf
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
