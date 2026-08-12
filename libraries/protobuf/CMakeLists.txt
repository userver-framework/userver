project(userver-protobuf CXX)

userver_module(
    protobuf
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
    INSTALL_COMPONENT grpc
    LINK_LIBRARIES protobuf::libprotobuf
    UTEST_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/tests"
    UTEST_LINK_LIBRARIES userver-protobuf-unittest-proto
)

if(USERVER_BUILD_TESTS)
    add_subdirectory(tests/proto)
endif()
