#pragma once

/// @file userver/tracing/tracer.hpp
/// @brief Tracing configuration for span logging suppression

#include <optional>
#include <string>

#include <dynamic_config/variables/USERVER_NO_LOG_SPANS.hpp>
#include <userver/logging/level.hpp>

USERVER_NAMESPACE_BEGIN

/// Tracing support via @ref tracing::Span
namespace tracing {

using NoLogSpans = ::dynamic_config::userver_no_log_spans::VariableType;

/// Sets the global configuration for disabling logging some of the @ref tracing::Span.
void SetNoLogSpans(NoLogSpans&& spans);

/// Returns true iff the @ref tracing::Span with `name` is not logged.
bool IsNoLogSpan(const std::string& name);

/// Sets the global minimum log level for spans. `std::nullopt` makes spans use
/// the default logger's level, preserving the legacy behavior.
void SetSpanLogLevel(std::optional<logging::Level> log_level) noexcept;

/// Returns the configured global span log level, or `std::nullopt` if spans
/// use the default logger's level.
std::optional<logging::Level> GetSpanLogLevel() noexcept;

/// Isolates tests and benchmarks by resetting span logging configuration on
/// construction: no suppressed spans and no span log level override.
/// Restores the previous configuration on destruction.
class TracingStateGuard final {
public:
    TracingStateGuard();
    ~TracingStateGuard();

    TracingStateGuard(const TracingStateGuard&) = delete;
    TracingStateGuard(TracingStateGuard&&) = delete;
    TracingStateGuard& operator=(const TracingStateGuard&) = delete;
    TracingStateGuard& operator=(TracingStateGuard&&) = delete;

private:
    NoLogSpans previous_no_log_spans_;
    std::optional<logging::Level> previous_span_log_level_;
};

}  // namespace tracing

USERVER_NAMESPACE_END
