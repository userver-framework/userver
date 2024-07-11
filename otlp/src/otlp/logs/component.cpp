#include <userver/otlp/logs/component.hpp>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/logging/component.hpp>
#include <userver/logging/level_serialization.hpp>
#include <userver/logging/log.hpp>
#include <userver/logging/null_logger.hpp>

#include <userver/ugrpc/client/client_factory_component.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include "logger.hpp"

#include <opentelemetry/proto/collector/logs/v1/logs_service_client.usrv.pb.hpp>

USERVER_NAMESPACE_BEGIN

namespace otlp {

LoggerComponent::LoggerComponent(const components::ComponentConfig& config,
                                 const components::ComponentContext& context) {
  auto& client_factory =
      context.FindComponent<ugrpc::client::ClientFactoryComponent>()
          .GetFactory();

  auto endpoint = config["endpoint"].As<std::string>();
  auto client = std::make_shared<
      opentelemetry::proto::collector::logs::v1::LogsServiceClient>(
      client_factory.MakeClient<
          opentelemetry::proto::collector::logs::v1::LogsServiceClient>(
          "otlp-logger", endpoint));

  auto trace_client = std::make_shared<
      opentelemetry::proto::collector::trace::v1::TraceServiceClient>(
      client_factory.MakeClient<
          opentelemetry::proto::collector::trace::v1::TraceServiceClient>(
          "otlp-tracer", endpoint));

  LoggerConfig logger_config;
  logger_config.max_queue_size = config["max-queue-size"].As<size_t>(65535);
  logger_config.max_batch_delay =
      config["max-batch-delay"].As<std::chrono::milliseconds>(100);
  logger_config.service_name =
      config["service-name"].As<std::string>("unknown_service");
  logger_config.log_level =
      config["log-level"].As<USERVER_NAMESPACE::logging::Level>();
  logger_ = std::make_shared<Logger>(std::move(client), std::move(trace_client),
                                     std::move(logger_config));

  // We must init after the default logger is initialized
  context.FindComponent<components::Logging>();

  logging::impl::SetDefaultLoggerRef(*logger_);
}

LoggerComponent::~LoggerComponent() {
  std::cerr << "Destroying default logger\n";
  logging::impl::SetDefaultLoggerRef(logging::GetNullLogger());
}

yaml_config::Schema LoggerComponent::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<components::RawComponentBase>(R"(
type: object
description: >
    OpenTelemetry logger component
additionalProperties: false
properties:
    endpoint:
        type: string
        description: >
            Hostname:port of otel collector (gRPC).
    log-level:
        type: string
        description: log level
    max-queue-size:
        type: integer
        description: max async queue size
    max-batch-delay:
        type: string
        description: max delay between send batches (e.g. 100ms or 1s)
    service-name:
        type: string
        description: service name
    attributes:
        type: object
        description: extra OTLP attributes
        properties: {}
        additionalProperties:
          type: string
          description: attribute value

)");
}

}  // namespace otlp

USERVER_NAMESPACE_END
