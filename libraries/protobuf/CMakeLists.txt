project(userver-protobuf CXX)

add_subdirectory(tests)

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
