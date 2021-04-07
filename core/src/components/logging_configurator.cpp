#include <components/logging_configurator.hpp>
#include <taxi_config/storage/component.hpp>
#include <tracing/no_log_spans.hpp>
#include <tracing/tracer.hpp>

#include <logging/rate_limit.hpp>

namespace components {

namespace {

tracing::NoLogSpans ParseNoLogSpans(const taxi_config::DocsMap& docs_map) {
  return docs_map.Get("USERVER_NO_LOG_SPANS").As<tracing::NoLogSpans>();
}

constexpr taxi_config::Key<ParseNoLogSpans> kNoLogSpans{};

}  // namespace

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
  tracing::Tracer::SetNoLogSpans(config_.GetCopy(kNoLogSpans));
}

}  // namespace components
