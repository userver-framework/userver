project(userver-otlp CXX)

include(GrpcTargets)
include(SetupOpentelemetryProto)

userver_module(otlp
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
    LINK_LIBRARIES userver-otlp-proto

    UTEST_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/tests"
    UTEST_LINK_LIBRARIES userver-grpc-utest
)

_userver_install_targets(COMPONENT otlp TARGETS userver-otlp-proto)
