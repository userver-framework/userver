set(_PROTO_GEN "${CMAKE_CURRENT_BINARY_DIR}/generated")

# protoc requires its output directory to already exist.
file(MAKE_DIRECTORY "${_PROTO_GEN}")

# This CMakeLists.txt lives in the proto root on purpose: protobuf_generate()
# computes the generated header subdirectory relative to CMAKE_CURRENT_SOURCE_DIR
# in some CMake/Protobuf versions and relative to IMPORT_DIRS in others. Keeping
# the proto root equal to CMAKE_CURRENT_SOURCE_DIR makes both behave the same and
# deterministically place the header at `${_PROTO_GEN}/proto_json/messages.pb.h`.
add_library(userver-protobuf-unittest-proto STATIC "${CMAKE_CURRENT_SOURCE_DIR}/proto_json/messages.proto")
protobuf_generate(
    TARGET userver-protobuf-unittest-proto
    LANGUAGE cpp
    IMPORT_DIRS
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${Protobuf_INCLUDE_DIR}"
    PROTOC_OUT_DIR "${_PROTO_GEN}"
)

target_link_libraries(userver-protobuf-unittest-proto PUBLIC protobuf::libprotobuf)
target_compile_options(userver-protobuf-unittest-proto PUBLIC -Wno-unused-parameter)
target_include_directories(
    userver-protobuf-unittest-proto SYSTEM PUBLIC $<BUILD_INTERFACE:${_PROTO_GEN}>
)
