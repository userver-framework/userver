project(userver-protobuf CXX)

userver_module(
    protobuf
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
    INSTALL_COMPONENT grpc
    LINK_LIBRARIES protobuf::libprotobuf userver::universal
    UTEST_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/datetime_test.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/json/*.cpp"
    UTEST_LINK_LIBRARIES userver-protobuf-unittest-proto
)

if(USERVER_BUILD_TESTS)
    set(_PROTO_SRC  "${CMAKE_CURRENT_SOURCE_DIR}/tests/proto/proto_json/messages.proto")
    set(_PROTO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/tests/proto")
    set(_PROTO_GEN  "${CMAKE_CURRENT_BINARY_DIR}/proto")

    # protoc requires its output directory to already exist.
    file(MAKE_DIRECTORY "${_PROTO_GEN}")

    add_library(userver-protobuf-unittest-proto STATIC "${_PROTO_SRC}")
    protobuf_generate(
        TARGET userver-protobuf-unittest-proto
        LANGUAGE cpp
        IMPORT_DIRS
            "${_PROTO_ROOT}"
            "${Protobuf_INCLUDE_DIR}"
        PROTOC_OUT_DIR "${_PROTO_GEN}"
    )

    target_compile_options(userver-protobuf-unittest-proto PUBLIC -Wno-unused-parameter)
    target_include_directories(userver-protobuf-unittest-proto SYSTEM PUBLIC
        $<BUILD_INTERFACE:${_PROTO_GEN}>
    )
    target_link_libraries(userver-protobuf-unittest-proto PUBLIC protobuf::libprotobuf)
endif()
