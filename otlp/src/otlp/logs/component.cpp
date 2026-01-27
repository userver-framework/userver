#include <userver/otlp/logs/component.hpp>

#include <string>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/logging/component.hpp>
#include <userver/logging/impl/mem_logger.hpp>
#include <userver/logging/level_serialization.hpp>
#include <userver/logging/log.hpp>
#include <userver/logging/null_logger.hpp>

#include <userver/ugrpc/client/client_factory_component.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include "logger.hpp"

#include <opentelemetry/proto/collector/logs/v1/logs_service_client.usrv.pb.hpp>

#ifndef ARCADIA_ROOT
#include "generated/src/otlp/logs/component.yaml.hpp"  // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace otlp {

LoggerComponent::LoggerComponent(const components::ComponentConfig& config, const components::ComponentContext& context)
    : old_logger_(logging::GetDefaultLogger())
{
    auto client_factory_name = config["client-factory-name"].As<std::string>();

    auto&
        client_factory = context.FindComponent<ugrpc::client::ClientFactoryComponent>(client_factory_name).GetFactory();

    std::string logs_endpoint;
    std::string tracing_endpoint;

    auto endpoint_cfg = config["endpoint"];
    auto logs_endpoint_cfg = config["logs-endpoint"];
    auto tracing_endpoint_cfg = config["tracing-endpoint"];

    auto is_present = [](auto&& x) { return !(x.IsMissing() || x.IsNull()); };

    if (is_present(endpoint_cfg)) {
        // we have one endpoint
        if (is_present(logs_endpoint_cfg)) {
            throw std::runtime_error(R"("endpoint" option is incompatible with "logs-endpoint")");
        }
        if (is_present(tracing_endpoint_cfg)) {
            throw std::runtime_error(R"("endpoint" option is incompatible with "tracing-endpoint")");
        }

        logs_endpoint = endpoint_cfg.As<std::string>();
        tracing_endpoint = endpoint_cfg.As<std::string>();
    } else {
        logs_endpoint = logs_endpoint_cfg.As<std::string>();
        tracing_endpoint = tracing_endpoint_cfg.As<std::string>();
    }

    auto client = client_factory.MakeClient<
        opentelemetry::proto::collector::logs::v1::LogsServiceClient>("otlp-logger", logs_endpoint);

    auto trace_client = client_factory.MakeClient<
        opentelemetry::proto::collector::trace::v1::TraceServiceClient>("otlp-tracer", tracing_endpoint);

    LoggerConfig logger_config;
    logger_config.max_queue_size = config["max-queue-size"].As<size_t>(65535);
    logger_config.max_batch_delay = config["max-batch-delay"].As<std::chrono::milliseconds>(100);
    logger_config.service_name = config["service-name"].As<std::string>("unknown_service");
    logger_config.log_level = config["log-level"].As<USERVER_NAMESPACE::logging::Level>();
    logger_config.extra_attributes = config["extra-attributes"].As<std::unordered_map<std::string, std::string>>({});
    logger_config.attributes_mapping = config["attributes-mapping"].As<std::unordered_map<std::string, std::string>>({}
    );
    logger_config.logs_sink = config["sinks"]["logs"].As<SinkType>(SinkType::kOtlp);
    logger_config.tracing_sink = config["sinks"]["tracing"].As<SinkType>(SinkType::kOtlp);

    logger_ = std::make_shared<Logger>(std::move(client), std::move(trace_client), std::move(logger_config));
    // We must init after the default logger is initialized
    auto& logging_component = context.FindComponent<components::Logging>();
    logging::LoggerPtr default_logger{};
    if (logger_config.logs_sink == SinkType::kOtlp && logger_config.tracing_sink == SinkType::kOtlp) {
        if (logging_component.GetLoggerOptional("default")) {
            throw std::runtime_error(
                "You've registered both the 'otlp-logger' component and the "
                "'default' logger in 'logging' component, but have opted to "
                "send both logs and traces using oltp. Either disable default "
                "logger or otlp logger."
            );
        }
    } else {
        try {
            default_logger = logging_component.GetLogger("default");
        } catch (const std::runtime_error&) {
            default_logger = nullptr;
        }
        if (!default_logger) {
            throw std::runtime_error(
                "You've opted to use the 'default' logger, but it's not registered "
                "in 'loggers' of the 'logging' component. Please register it, or "
                "your logs and/or traces won't be saved."
            );
        }
    }

    logger_->SetDefaultLogger(std::move(default_logger));

    logging::impl::SetDefaultLoggerRef(*logger_);
    old_logger_.ForwardTo(&*logger_);

    auto* const statistics_storage = context.FindComponentOptional<components::StatisticsStorage>();
    if (statistics_storage) {
        statistics_holder_ =
            statistics_storage->GetStorage().RegisterWriter("logger", [this](utils::statistics::Writer& writer) {
                writer.ValueWithLabels(logger_->GetStatistics(), {"logger", "default"});
            });
    }
}

LoggerComponent::~LoggerComponent() {
    old_logger_.ForwardTo(nullptr);
    logging::impl::SetDefaultLoggerRef(old_logger_);

    logger_->Stop();

    static std::shared_ptr<Logger> logger_pin;
    logger_pin = logger_;
}

yaml_config::Schema LoggerComponent::GetStaticConfigSchema() {
    return yaml_config::MergeSchemasFromResource<components::RawComponentBase>("src/otlp/logs/component.yaml");
}

}  // namespace otlp

USERVER_NAMESPACE_END
