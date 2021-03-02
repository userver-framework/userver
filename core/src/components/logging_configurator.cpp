#include <components/logging_configurator.hpp>
#include <taxi_config/storage/component.hpp>
#include <tracing/no_log_spans.hpp>
#include <tracing/tracer.hpp>

#include <logging/rate_limit.hpp>

namespace components {

struct LoggingConfiguratorConfig {
  using DocsMap = taxi_config::DocsMap;

  explicit LoggingConfiguratorConfig(const DocsMap& docs_map)
      : no_log_spans(
            docs_map.Get("USERVER_NO_LOG_SPANS").As<tracing::NoLogSpans>()) {}

  tracing::NoLogSpans no_log_spans;
};

LoggingConfigurator::LoggingConfigurator(const ComponentConfig& config,
                                         const ComponentContext& context)
    : config_(context) {
  logging::impl::SetLogLimitedEnable(
      config["limited-logging-enable"].As<bool>());
  logging::impl::SetLogLimitedInterval(
      config["limited-logging-interval"].As<std::chrono::milliseconds>());

  config_subscription_ = taxi_config::UpdateAndListen(
      context, this, kName, &LoggingConfigurator::OnConfigUpdate);
}

void LoggingConfigurator::OnConfigUpdate() {
  const auto config = config_.Get();
  tracing::Tracer::SetNoLogSpans(tracing::NoLogSpans{config->no_log_spans});
}

}  // namespace components
