#include <components/logging_configurator.hpp>
#include <taxi_config/storage/component.hpp>
#include <tracing/no_log_spans.hpp>
#include <tracing/tracer.hpp>

namespace components {

struct LoggingConfiguratorConfig {
  using DocsMap = taxi_config::DocsMap;

  explicit LoggingConfiguratorConfig(const DocsMap& docs_map)
      : no_log_spans(
            docs_map.Get("USERVER_NO_LOG_SPANS").As<tracing::NoLogSpans>()) {}

  tracing::NoLogSpans no_log_spans;
};

LoggingConfigurator::LoggingConfigurator(const ComponentConfig&,
                                         const ComponentContext& context) {
  auto& config_component = context.FindComponent<::components::TaxiConfig>();
  config_subscription_ = config_component.UpdateAndListen(
      this, "update_logging_config", &LoggingConfigurator::OnConfigUpdate);
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void LoggingConfigurator::OnConfigUpdate(
    const std::shared_ptr<const taxi_config::Config>& config) {
  auto new_config = config->Get<LoggingConfiguratorConfig>().no_log_spans;
  tracing::Tracer::SetNoLogSpans(std::move(new_config));
}

}  // namespace components
