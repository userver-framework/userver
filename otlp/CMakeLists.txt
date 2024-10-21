project(userver-otlp CXX)

include(GrpcTargets)
include(SetupOpentelemetryProto)

find_package(Boost REQUIRED regex)

userver_module(otlp
  SOURCE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}
  LINK_LIBRARIES
    userver-otlp-proto
  UTEST_LINK_LIBRARIES
    userver-grpc-utest
)

_userver_install_targets(COMPONENT otlp TARGETS userver-otlp-proto)
