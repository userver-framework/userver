#pragma once

/// @file userver/logging/telemetry_settings.hpp
/// @brief Policies for @ref components::LoggingConfigurator.

#include <userver/formats/json_fwd.hpp>
#include <userver/logging/level.hpp>
#include <userver/utils/default_dict.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {

/// Settings for one logger. Default values impose no additional restrictions.
struct TelemetrySettings final {
    /// Minimum severity, combined with the logger's level captured at registration.
    Level min_log_level{Level::kTrace};
    /// Fraction of logs below warning to retain, in [0, 1]; excludes spans and force-enabled logs.
    double log_sample_rate{1.0};

    /// Returns the higher minimum level and lower sampling rate without modifying either input.
    /// Both inputs must satisfy @ref logging::TelemetrySettings::Validate.
    [[nodiscard]] TelemetrySettings Merge(const TelemetrySettings& other) const;

    /// @throws std::runtime_error if the sampling rate is non-finite or outside [0, 1].
    void Validate() const;

    /// Compares all settings.
    bool operator==(const TelemetrySettings&) const = default;
};

/// Parses required min_log_level and log_sample_rate fields and validates the result.
/// Missing or invalid fields are rejected; unknown fields are ignored.
TelemetrySettings Parse(const formats::json::Value& value, formats::parse::To<TelemetrySettings>);

/// Process-wide trace sampling, independent of logger sampling.
struct TracingSettings final {
    /// Fraction of traces to retain in [0, 1]; 1 imposes no sampling restriction.
    double trace_sample_rate{1.0};

    /// Returns the lower sampling rate without modifying either input.
    /// Both inputs must satisfy @ref logging::TracingSettings::Validate.
    [[nodiscard]] TracingSettings Merge(const TracingSettings& other) const;

    /// @throws std::runtime_error if the sampling rate is non-finite or outside [0, 1].
    void Validate() const;

    /// Compares all settings.
    bool operator==(const TracingSettings&) const = default;
};

/// Parses the required trace_sample_rate field and validates the result.
/// Missing or invalid fields are rejected; unknown fields are ignored.
TracingSettings Parse(const formats::json::Value& value, formats::parse::To<TracingSettings>);

/// Combined policy accepted by @ref components::LoggingConfigurator::SetRestrictions.
struct TelemetryConfig final {
    /// Logger name to settings; __default__ is used only for names without explicit entries.
    utils::DefaultDict<TelemetrySettings> loggers;
    /// Trace settings shared by the whole process.
    TracingSettings tracing;

    /// Validates every logger entry, including __default__, and the trace settings.
    /// @throws std::runtime_error if any sampling rate is non-finite or outside [0, 1].
    void Validate() const;

    /// Compares both policies.
    bool operator==(const TelemetryConfig&) const = default;
};

/// Parses the optional logging and tracing sections of @ref USERVER_TELEMETRY_SETTINGS.
/// Missing sections impose no restrictions. Invalid fields are rejected;
/// unknown fields and sections are ignored.
TelemetryConfig Parse(const formats::json::Value& value, formats::parse::To<TelemetryConfig>);

}  // namespace logging

USERVER_NAMESPACE_END
