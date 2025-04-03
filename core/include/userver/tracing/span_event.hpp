#pragma once

/// @file userver/tracing/span_event.hpp
/// @brief @copybrief tracing::SpanEvent

#include <chrono>
#include <string_view>
#include <unordered_map>

#include <userver/formats/json_fwd.hpp>
#include <userver/tracing/any_value.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {

/// @brief Span event -- time-stamped annotation of the span with user-provided text description.
/// @see https://opentelemetry.io/docs/concepts/signals/traces/#span-events
/// @see
/// https://github.com/open-telemetry/opentelemetry-proto/blob/v1.3.2/opentelemetry/proto/trace/v1/trace.proto#L220.
struct SpanEvent final {
    using Timestamp = std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>;
    using KeyValue = std::unordered_map<std::string, AnyValue>;

    /// @brief Constructor. Creates span event with name and timestamp.
    /// @param name Event name.
    explicit SpanEvent(std::string_view name);

    /// @brief Constructor. Creates span event with name, timestamp and attributes.
    /// @param name Event name.
    /// @param attributes Key-value attributes.
    SpanEvent(std::string_view name, KeyValue attributes);

    /// @brief Event name.
    std::string name;

    /// @brief Event timestamp.
    Timestamp timestamp{};

    /// @brief Attributes.
    /// @see
    /// https://github.com/open-telemetry/opentelemetry-proto/blob/v1.3.2/opentelemetry/proto/common/v1/common.proto#L64.
    /// @see
    /// https://github.com/open-telemetry/opentelemetry-proto/blob/v1.3.2/opentelemetry/proto/trace/v1/trace.proto#L231.
    /// @details Collection of unique key-value pairs.
    KeyValue attributes;
};

SpanEvent Parse(const formats::json::Value& value, formats::parse::To<SpanEvent>);

void WriteToStream(const SpanEvent& span_event, formats::json::StringBuilder& sw);

}  // namespace tracing

USERVER_NAMESPACE_END
