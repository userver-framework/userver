#include <userver/components/logging_configurator.hpp>

#include <logging/dynamic_debug.hpp>
#include <logging/dynamic_debug_config.hpp>
#include <tracing/no_log_spans.hpp>
#include <userver/components/component.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/dynamic_config/value.hpp>
#include <userver/tracing/tracer.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <logging/rate_limit.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

namespace {

/// [key]
tracing::NoLogSpans ParseNoLogSpans(const dynamic_config::DocsMap& docs_map) {
  return docs_map.Get("USERVER_NO_LOG_SPANS").As<tracing::NoLogSpans>();
}

constexpr dynamic_config::Key<ParseNoLogSpans> kNoLogSpans{};
/// [key]

logging::DynamicDebugConfig ParseDynamicDebug(
    const dynamic_config::DocsMap& docs_map) {
  return docs_map.Get("USERVER_LOG_DYNAMIC_DEBUG")
      .As<logging::DynamicDebugConfig>();
}

constexpr dynamic_config::Key<ParseDynamicDebug> kDynamicDebugConfig{};

}  // namespace

LoggingConfigurator::LoggingConfigurator(const ComponentConfig& config,
                                         const ComponentContext& context)
    : dynamic_debug_({}) {
  logging::impl::SetLogLimitedEnable(
      config["limited-logging-enable"].As<bool>());
  logging::impl::SetLogLimitedInterval(
      config["limited-logging-interval"].As<std::chrono::milliseconds>());

  config_subscription_ =
      context.FindComponent<components::DynamicConfig>()
          .GetSource()
          .UpdateAndListen(this, kName, &LoggingConfigurator::OnConfigUpdate);
}

LoggingConfigurator::~LoggingConfigurator() {
  config_subscription_.Unsubscribe();
}

void LoggingConfigurator::OnConfigUpdate(
    const dynamic_config::Snapshot& config) {
  (void)this;  // silence clang-tidy
  tracing::Tracer::SetNoLogSpans(tracing::NoLogSpans{config[kNoLogSpans]});

  const auto& dd = config[kDynamicDebugConfig];
  auto old_dd = dynamic_debug_.Read();
  if (!(*old_dd == dd)) {
    auto lock = dynamic_debug_.StartWrite();
    *lock = dd;

    /* There is a race between multiple AddDynamicDebugLog(), thus some logs
     * may be logged or not logged by mistake. This is on purpose as logging
     * locking would be too slow and heavy.
     */

    // Flush
    AddDynamicDebugLog("", logging::kAnyLine, logging::EntryState::kDefault);

    for (const auto& location : dd.force_disabled) {
      AddDynamicDebugLog(location, logging::kAnyLine,
                         logging::EntryState::kForceDisabled);
    }
    for (const auto& location : dd.force_enabled) {
      AddDynamicDebugLog(location, logging::kAnyLine,
                         logging::EntryState::kForceEnabled);
    }

    lock.Commit();
  }
}

yaml_config::Schema LoggingConfigurator::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<impl::ComponentBase>(R"(
type: object
description: Helper component to configure logging
additionalProperties: false
properties:
    limited-logging-enable:
        type: boolean
        description: set to true to make LOG_LIMITED drop repeated logs
    limited-logging-interval:
        type: string
        description: utils::StringToDuration suitable duration string to group repeated logs into one message
)");
}

}  // namespace components

USERVER_NAMESPACE_END
