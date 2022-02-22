#include <userver/components/logging_configurator.hpp>

#include <tracing/no_log_spans.hpp>
#include <userver/components/component.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/dynamic_config/value.hpp>
#include <userver/tracing/tracer.hpp>

#include <logging/rate_limit.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

namespace {

/// [LoggingConfigurator config key]
tracing::NoLogSpans ParseNoLogSpans(const dynamic_config::DocsMap& docs_map) {
  return docs_map.Get("USERVER_NO_LOG_SPANS").As<tracing::NoLogSpans>();
}

constexpr dynamic_config::Key<ParseNoLogSpans> kNoLogSpans{};
/// [LoggingConfigurator config key]

}  // namespace

LoggingConfigurator::LoggingConfigurator(const ComponentConfig& config,
                                         const ComponentContext& context) {
  logging::impl::SetLogLimitedEnable(
      config["limited-logging-enable"].As<bool>());
  logging::impl::SetLogLimitedInterval(
      config["limited-logging-interval"].As<std::chrono::milliseconds>());

  config_subscription_ =
      context.FindComponent<components::TaxiConfig>()
          .GetSource()
          .UpdateAndListen(this, kName, &LoggingConfigurator::OnConfigUpdate);
}

void LoggingConfigurator::OnConfigUpdate(
    const dynamic_config::Snapshot& config) {
  (void)this;  // silence clang-tidy
  tracing::Tracer::SetNoLogSpans(tracing::NoLogSpans{config[kNoLogSpans]});
}

}  // namespace components

USERVER_NAMESPACE_END
