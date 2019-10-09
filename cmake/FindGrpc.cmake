find_path(GRPC_INCLUDE_DIRECTORY "grpc/grpc.h")
find_path(PROTOBUF_INCLUDE_DIRECTORY "google/protobuf/port_def.inc" PATH_SUFFIXES "yandex-taxi-proto3")
find_library(GRPC_LIBRARY grpc)
find_library(GRPCPP_LIBRARY grpc++)
find_library(GRPC_PROTOBUF_LIBRARY NAMES yandex-taxi-protobuf protobuf)
find_library(GPR_LIBRARY gpr)
find_library(CARES_LIBRARY NAMES yandex-taxi-cares cares)
find_library(ADDRESS_SORTING_LIBRARY address_sorting)

include(FindPackageHandleStandardArgs)
set(DEP_LIST
  libyandex-taxi-protobuf-dev
  libyandex-taxi-protobuf1
  libyandex-taxi-protoc1
  libyandex-taxi-protoc-dev
  yandex-taxi-protobuf-compiler
  libyandex-taxi-grpc-dev
  libyandex-taxi-grpc7
  libyandex-taxi-grpc++-dev
  libyandex-taxi-grpc++1
  yandex-taxi-protobuf-compiler-grpc
)
set(DEP_MESSAGE)
foreach(dep ${DEP_LIST})
  set(DEP_MESSAGE "${DEP_MESSAGE} ${dep}")
endforeach(dep ${DEP_LIST})
find_package_handle_standard_args(Grpc
    "Can't find libgrpc, please install `${DEP_MESSAGE}` or `brew install protobuf grpc`. \
    Also python package protobuf is requried."
    GRPC_INCLUDE_DIRECTORY
    PROTOBUF_INCLUDE_DIRECTORY
    GPR_LIBRARY
    GRPC_LIBRARY
    GRPCPP_LIBRARY
    CARES_LIBRARY
    ADDRESS_SORTING_LIBRARY
    GRPC_PROTOBUF_LIBRARY)

if (Grpc_FOUND)
    set(GRPC_INCLUDE_DIRECTORIES
      ${GRPC_INCLUDE_DIRECTORY}
      ${PROTOBUF_INCLUDE_DIRECTORY}
      CACHE STRING "Include directories for grpc library"
    )
    set(GRPC_PROTO_INCLUDE_DIRECTORIES
      ${PROTOBUF_INCLUDE_DIRECTORY}
      CACHE STRING "Include directories for proto files"
    )
    set(GRPC_LIBRARIES
      ${GRPCPP_LIBRARY}
      ${GRPC_LIBRARY}
      ${GPR_LIBRARY}
      ${CARES_LIBRARY}
      ${ADDRESS_SORTING_LIBRARY}
      ${GRPC_PROTOBUF_LIBRARY}
      CACHE STRING "Link libraries for grpc"
    )
    message(STATUS "GRPC include dirs ${GRPC_INCLUDE_DIRECTORIES}")
    if (NOT TARGET Grpc)
        add_library(Grpc INTERFACE IMPORTED)
    endif()
    set_property(TARGET Grpc PROPERTY
        INTERFACE_INCLUDE_DIRECTORIES "${GRPC_INCLUDE_DIRECTORIES}")
    set_property(TARGET Grpc PROPERTY
        INTERFACE_LINK_LIBRARIES "${GRPC_LIBRARIES}")
endif()

mark_as_advanced(GRPC_INCLUDE_DIRECTORIES GRPC_LIBRARIES)
