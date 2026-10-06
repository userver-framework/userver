#pragma once

/// @file userver/tracing/tracer.hpp
/// @brief Tracing configuration for span logging suppression

#include <optional>
#include <string>
#include <string_view>

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

/// @brief Sets the probability with which traces are sampled. Intended for
/// service degradation under high load. Passing `std::nullopt` disables the
/// sampling (all traces are sampled), values are clamped to [0.0, 1.0]
/// and rounded down to a multiple of 2^-64. NaN disables sampling.
///
/// The decision is made once per trace at the root @ref tracing::Span of the
/// request (http/grpc handlers, tasks, etc.) deterministically by the trace id
/// and is inherited by child spans, so spans of a single trace are either all
/// written or all skipped. An explicit @ref tracing::Span::SetSampled or
/// @ref tracing::SpanBuilder::SetSampled call (e.g. from an incoming
/// opentelemetry `sampled` flag) overrides the decision.
void SetTraceSamplingProbability(std::optional<double> probability) noexcept;

/// @brief Returns the current trace sampling probability, `std::nullopt` means
/// the sampling is disabled.
std::optional<double> GetTraceSamplingProbability() noexcept;

namespace impl {

/// Per-trace sampling decision, deterministic by the trace id.
bool ShouldSampleTrace(std::string_view trace_id) noexcept;

}  // namespace impl

/// Isolates tests and benchmarks by resetting span logging configuration on
/// construction: no suppressed spans, no span log level override and no sampling.
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
    std::optional<double> previous_trace_sampling_probability_;
};

}  // namespace tracing

USERVER_NAMESPACE_END
