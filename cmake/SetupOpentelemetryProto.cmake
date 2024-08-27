include_guard(GLOBAL)


set(USERVER_OPENTELEMETRY_PROTO "" CACHE PATH "Path to the folder with opentelemetry proto files")
if (NOT USERVER_OPENTELEMETRY_PROTO)
    include(DownloadUsingCPM)
    CPMAddPackage(
      NAME opentelemetry_proto
      GITHUB_REPOSITORY open-telemetry/opentelemetry-proto
      VERSION 1.3.2
      DOWNLOAD_ONLY
    )
else()
    set(opentelemetry_proto_SOURCE_DIR ${USERVER_OPENTELEMETRY_PROTO})
endif()
message(STATUS "Opentelemetry proto path: ${opentelemetry_proto_SOURCE_DIR}")

userver_add_grpc_library(userver-otlp-proto
  SOURCE_PATH
    ${opentelemetry_proto_SOURCE_DIR}
  PROTOS
    ${opentelemetry_proto_SOURCE_DIR}/opentelemetry/proto/collector/trace/v1/trace_service.proto
    ${opentelemetry_proto_SOURCE_DIR}/opentelemetry/proto/collector/logs/v1/logs_service.proto
    ${opentelemetry_proto_SOURCE_DIR}/opentelemetry/proto/common/v1/common.proto
    ${opentelemetry_proto_SOURCE_DIR}/opentelemetry/proto/logs/v1/logs.proto
    ${opentelemetry_proto_SOURCE_DIR}/opentelemetry/proto/resource/v1/resource.proto
    ${opentelemetry_proto_SOURCE_DIR}/opentelemetry/proto/trace/v1/trace.proto
)

