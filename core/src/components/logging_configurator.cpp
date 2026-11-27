#include <userver/components/logging_configurator.hpp>

#include <chrono>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>

#include <logging/dynamic_debug.hpp>
#include <logging/dynamic_debug_config.hpp>
#include <userver/alerts/source.hpp>
#include <userver/components/component.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/dynamic_config/source.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/dynamic_config/value.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/logging/component.hpp>
#include <userver/logging/log.hpp>
#include <userver/logging/telemetry_settings.hpp>
#include <userver/tracing/tracer.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <logging/rate_limit.hpp>
#include <logging/split_location.hpp>

#include <dynamic_config/variables/USERVER_LOG_DYNAMIC_DEBUG.hpp>
#include <dynamic_config/variables/USERVER_NO_LOG_SPANS.hpp>
#include <dynamic_config/variables/USERVER_TELEMETRY_SETTINGS.hpp>

#ifndef ARCADIA_ROOT
#include "generated/src/components/logging_configurator.yaml.hpp"  // Y_IGNORE
#endif

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
)"},
    ::dynamic_config::userver_log_dynamic_debug::GetSchemaHash()
};

alerts::Source kDynamicDebugInvalidLocation{"dynamic_debug_invalid_location"};

const dynamic_config::Key<logging::TelemetryConfig> kTelemetrySettings{
    "USERVER_TELEMETRY_SETTINGS",
    dynamic_config::DefaultAsJsonString{"{}"},
    ::dynamic_config::userver_telemetry_settings::GetSchemaHash()
};

}  // namespace

class LoggingConfigurator::TelemetryState final {
public:
    explicit TelemetryState(Logging& logging)
        : logging_(logging),
          initial_trace_sampling_(tracing::GetTraceSamplingProbability())
    {}

    ~TelemetryState() {
        logging_.RestoreTelemetrySettings();
        tracing::SetTraceSamplingProbability(initial_trace_sampling_);
    }

    void SetSettings(logging::TelemetryConfig settings) {
        const std::lock_guard lock{mutex_};
        settings_ = std::move(settings);
        ApplyTelemetrySettings(lock);
    }

    void SetRestrictions(logging::TelemetryConfig restrictions) {
        restrictions.Validate();
        const std::lock_guard lock{mutex_};
        restrictions_ = std::move(restrictions);
        ApplyTelemetrySettings(lock);
    }

private:
    void ApplyTelemetrySettings(const std::lock_guard<engine::Mutex>&) const {
        logging_.SetTelemetrySettings(settings_.loggers, restrictions_.loggers);
        const auto rate = settings_.tracing.Merge(restrictions_.tracing).trace_sample_rate;
        tracing::SetTraceSamplingProbability(rate == 1.0 ? std::nullopt : std::optional{rate});
    }

    engine::Mutex mutex_;
    Logging& logging_;
    logging::TelemetryConfig settings_;
    logging::TelemetryConfig restrictions_;
    const std::optional<double> initial_trace_sampling_;
};

LoggingConfigurator::LoggingConfigurator(const ComponentConfig& config, const ComponentContext& context)
    : metrics_storage_(context.FindComponent<components::StatisticsStorage>().GetMetricsStorage()),
      telemetry_(std::make_unique<TelemetryState>(context.FindComponent<components::Logging>()))
{
    logging::impl::SetLogLimitedEnable(config["limited-logging-enable"].As<bool>());
    logging::impl::SetLogLimitedInterval(config["limited-logging-interval"].As<std::chrono::milliseconds>());

    context.FindComponent<components::DynamicConfig>()
        .GetSource()
        .UpdateAndListen(context.Scopes(), this, kName, &LoggingConfigurator::OnConfigUpdate);
}

LoggingConfigurator::~LoggingConfigurator() = default;

void LoggingConfigurator::SetRestrictions(logging::TelemetryConfig restrictions) {
    telemetry_->SetRestrictions(std::move(restrictions));
}

void LoggingConfigurator::OnConfigUpdate(const dynamic_config::Diff& diff) {
    if (!diff.HasConfigsChanged(::dynamic_config::USERVER_NO_LOG_SPANS, kDynamicDebugConfig, kTelemetrySettings)) {
        return;
    }
    const auto& config = diff.current;
    if (diff.HasConfigsChanged(kTelemetrySettings)) {
        telemetry_->SetSettings(config[kTelemetrySettings]);
    }
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
            SetDynamicDebugLog(path, line, state);
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
            SetDynamicDebugLog(path, line, state);
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
    return yaml_config::MergeSchemasFromResource<RawComponentBase>("src/components/logging_configurator.yaml");
}

}  // namespace components

USERVER_NAMESPACE_END
