project(userver-grpc-protovalidate CXX)

# TODO: this is an early version of CMakeLists.txt. It is known to not work due to different issues. PRs with fixes are
# welcome!

include(SetupProtovalidate)

userver_module(
    grpc-protovalidate
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
    LINK_LIBRARIES userver::grpc protovalidate_cc::protovalidate_cc
    UTEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp"
    UTEST_LINK_LIBRARIES ${PROJECT_NAME}-unittest-proto userver-grpc-utest
    UBENCH_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks"
    UBENCH_LINK_LIBRARIES ${PROJECT_NAME}-unittest-proto
    EMBED_FILES src/grpc-protovalidate/client/component.yaml src/grpc-protovalidate/server/component.yaml
)

if(USERVER_BUILD_TESTS)
    # Installed protovalidate-cc provides headers, but test generation also needs validate.proto.
    find_path(
        USERVER_PROTOVALIDATE_PROTO_DIR buf/validate/validate.proto
        HINTS "${protovalidate_SOURCE_DIR}/proto/protovalidate"
        DOC "Directory containing protovalidate's buf/validate/validate.proto"
    )
    if(NOT USERVER_PROTOVALIDATE_PROTO_DIR)
        message(FATAL_ERROR "Set USERVER_PROTOVALIDATE_PROTO_DIR to build grpc-protovalidate tests and benchmarks")
    endif()
    userver_add_grpc_library(
        ${PROJECT_NAME}-unittest-proto
        SOURCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/tests/proto"
        PROTOS types/constrained.proto types/extensions.proto types/unit_test.proto
        INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/tests/proto" "${USERVER_PROTOVALIDATE_PROTO_DIR}"
    )
    target_link_libraries(${PROJECT_NAME}-unittest-proto PUBLIC protovalidate_cc::protovalidate_cc)
endif()
