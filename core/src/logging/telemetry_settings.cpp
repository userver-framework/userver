#include <userver/logging/telemetry_settings.hpp>

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <string_view>

#include <userver/formats/common/items.hpp>
#include <userver/formats/json/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {
namespace {

constexpr std::string_view kLoggingSection = "logging";
constexpr std::string_view kTracingSection = "tracing";

void ValidateRate(double rate, std::string_view field) {
    if (!std::isfinite(rate) || rate < 0.0 || rate > 1.0) {
        throw std::runtime_error(std::string{field} + " must be finite and in [0, 1]");
    }
}

}  // namespace

TelemetrySettings TelemetrySettings::Merge(const TelemetrySettings& other) const {
    return {
        .min_log_level = std::max(min_log_level, other.min_log_level),
        .log_sample_rate = std::min(log_sample_rate, other.log_sample_rate),
    };
}

void TelemetrySettings::Validate() const { ValidateRate(log_sample_rate, "log_sample_rate"); }

TelemetrySettings Parse(const formats::json::Value& value, formats::parse::To<TelemetrySettings>) {
    const TelemetrySettings settings{
        .min_log_level = LevelFromString(value["min_log_level"].As<std::string>()),
        .log_sample_rate = value["log_sample_rate"].As<double>(),
    };
    settings.Validate();
    return settings;
}

TracingSettings TracingSettings::Merge(const TracingSettings& other) const {
    return {.trace_sample_rate = std::min(trace_sample_rate, other.trace_sample_rate)};
}

void TracingSettings::Validate() const { ValidateRate(trace_sample_rate, "trace_sample_rate"); }

TracingSettings Parse(const formats::json::Value& value, formats::parse::To<TracingSettings>) {
    const TracingSettings settings{.trace_sample_rate = value["trace_sample_rate"].As<double>()};
    settings.Validate();
    return settings;
}

void TelemetryConfig::Validate() const {
    for (const auto& [name, settings] : loggers) {
        settings.Validate();
    }
    tracing.Validate();
}

TelemetryConfig Parse(const formats::json::Value& value, formats::parse::To<TelemetryConfig>) {
    value.CheckObject();
    TelemetryConfig settings;
    if (const auto logging = value[kLoggingSection]; !logging.IsMissing()) {
        logging.CheckObject();
        for (const auto& [name, entry] : formats::common::Items(logging)) {
            settings.loggers.Set(name, entry.As<TelemetrySettings>());
        }
    }
    if (const auto tracing = value[kTracingSection]; !tracing.IsMissing()) {
        settings.tracing = tracing.As<TracingSettings>();
    }
    return settings;
}

}  // namespace logging

USERVER_NAMESPACE_END
