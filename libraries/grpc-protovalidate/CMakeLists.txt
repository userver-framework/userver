project(userver-grpc-protovalidate CXX)

# TODO: this is an early version of CMakeLists.txt. It is known to not work due to different issues. PRs with fixes are
# wellcome!

include(SetupProtovalidate)

userver_module(
    grpc-protovalidate
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
    LINK_LIBRARIES userver::grpc protovalidate_cc::protovalidate_cc
    UTEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp"
)
