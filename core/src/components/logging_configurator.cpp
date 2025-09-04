#include <userver/components/logging_configurator.hpp>

#include <logging/dynamic_debug.hpp>
#include <logging/dynamic_debug_config.hpp>
#include <userver/alerts/source.hpp>
#include <userver/components/component.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/dynamic_config/value.hpp>
#include <userver/tracing/tracer.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <logging/rate_limit.hpp>
#include <logging/split_location.hpp>

#include <dynamic_config/variables/USERVER_NO_LOG_SPANS.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

namespace {

const dynamic_config::Key<logging::DynamicDebugConfig> kDynamicDebugConfig{
    "USERVER_LOG_DYNAMIC_DEBUG",
    dynamic_config::DefaultAsJsonString{R"(
  {
    "force-disabled": [],
    "force-enabled": []
  }
)"}};

alerts::Source kDynamicDebugInvalidLocation{"dynamic_debug_invalid_location"};

}  // namespace

LoggingConfigurator::LoggingConfigurator(const ComponentConfig& config, const ComponentContext& context)
    : metrics_storage_(context.FindComponent<components::StatisticsStorage>().GetMetricsStorage()) {
    logging::impl::SetLogLimitedEnable(config["limited-logging-enable"].As<bool>());
    logging::impl::SetLogLimitedInterval(config["limited-logging-interval"].As<std::chrono::milliseconds>());

    config_subscription_ = context.FindComponent<components::DynamicConfig>().GetSource().UpdateAndListen(
        this, kName, &LoggingConfigurator::OnConfigUpdate, ::dynamic_config::USERVER_NO_LOG_SPANS, kDynamicDebugConfig
    );
}

LoggingConfigurator::~LoggingConfigurator() { config_subscription_.Unsubscribe(); }

void LoggingConfigurator::OnConfigUpdate(const dynamic_config::Snapshot& config) {
    (void)this;  // silence clang-tidy
    tracing::SetNoLogSpans(tracing::NoLogSpans{config[::dynamic_config::USERVER_NO_LOG_SPANS]});

    const auto& dd = config[kDynamicDebugConfig];

    /* There is a race between multiple AddDynamicDebugLog(), thus some logs
     * may be logged or not logged by mistake. This is on purpose as logging
     * locking would be too slow and heavy.
     */

    // Flush
    logging::RemoveAllDynamicDebugLog();
    bool has_error = false;

    for (const auto& [location, level] : dd.force_disabled) {
        try {
            const auto [path, line] = logging::SplitLocation(location);
            logging::EntryState state;
            state.force_disabled_level_plus_one = logging::GetForceDisabledLevelPlusOne(level);
            AddDynamicDebugLog(path, line, state);
        } catch (const std::exception& e) {
            LOG_ERROR() << "Failed to disable dynamic debug log at '" << location << "': " << e;
            has_error = true;
        }
    }

    for (const auto& [location, level] : dd.force_enabled) {
        try {
            const auto [path, line] = logging::SplitLocation(location);
            logging::EntryState state;
            state.force_enabled_level = level;
            AddDynamicDebugLog(path, line, state);
        } catch (const std::exception& e) {
            LOG_ERROR() << "Failed to enable dynamic debug log at '" << location << "': " << e;
            has_error = true;
        }
    }

    if (has_error) {
        kDynamicDebugInvalidLocation.FireAlert(*metrics_storage_, alerts::Source::kInfiniteDuration);
    } else {
        kDynamicDebugInvalidLocation.StopAlertNow(*metrics_storage_);
    }
}

yaml_config::Schema LoggingConfigurator::GetStaticConfigSchema() {
    return yaml_config::MergeSchemas<RawComponentBase>(R"(
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
