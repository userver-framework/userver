#pragma once

/// @file userver/tracing/span_log_context.hpp
/// @brief @copybrief tracing::SpanLogContext

#include <cstddef>
#include <iosfwd>
#include <optional>
#include <string_view>

#include <userver/compiler/impl/lifetime.hpp>
#include <userver/logging/log_extra.hpp>
#include <userver/tracing/fwd.hpp>
#include <userver/utils/small_string.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {

/// @brief Owns a snapshot of a span's tracing identifiers and sampling state for logging.
///
/// Useful in rare cases when logs need to be associated with a span that has
/// already finished, such as request or response send errors, or logs from a
/// background task spawned by a request handler.
///
/// Capture the context with SetFromSpan() while the span is alive, then append
/// GetLogExtra() to a log record to attach the captured tracing fields.
class SpanLogContext final {
public:
    /// Replaces the captured context with the span's current logging context.
    /// Uses Span::GetSpanIdForChildLogs() to select the span ID, which may belong
    /// to a parent span if the supplied span is not loggable.
    void SetFromSpan(const Span& span);

    /// Returns the captured trace ID, or an empty string if no ID was captured.
    std::string_view GetTraceId() const noexcept USERVER_IMPL_LIFETIME_BOUND;
    /// Returns the captured logging span ID, or an empty string if no ID was captured.
    std::string_view GetSpanId() const noexcept USERVER_IMPL_LIFETIME_BOUND;
    /// Returns the captured link, or an empty string if no link was captured.
    std::string_view GetLink() const noexcept USERVER_IMPL_LIFETIME_BOUND;

    /// Returns true if no context has been captured with SetFromSpan().
    bool IsEmpty() const noexcept;
    /// Returns the captured sampling state, or false for an empty context.
    bool IsSampled() const noexcept;

    /// Returns the captured tracing fields for logging, or an empty LogExtra
    /// if no context has been captured.
    logging::LogExtra GetLogExtra() const;

    bool operator==(const SpanLogContext&) const = default;

private:
    static constexpr std::size_t kTraceIdInlineSize = 32;
    static constexpr std::size_t kSpanIdInlineSize = 16;
    static constexpr std::size_t kLinkInlineSize = 32;

    utils::SmallString<kTraceIdInlineSize> trace_id_;
    utils::SmallString<kSpanIdInlineSize> span_id_;
    utils::SmallString<kLinkInlineSize> link_;
    std::optional<bool> trace_sampled_;
};

void PrintTo(const SpanLogContext& context, std::ostream* os);

}  // namespace tracing

USERVER_NAMESPACE_END
