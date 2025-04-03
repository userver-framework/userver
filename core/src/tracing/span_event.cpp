#include <userver/tracing/span_event.hpp>

#include <fmt/format.h>

#include <userver/formats/json/string_builder.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/formats/serialize/write_to_stream.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {

namespace {

constexpr std::string_view kName = "name";
constexpr std::string_view kTimeUnixNano = "time_unix_nano";
constexpr std::string_view kAttributes = "attributes";

}  // namespace

SpanEvent::SpanEvent(std::string_view name) : name{name}, timestamp{std::chrono::system_clock::now()} {}

SpanEvent::SpanEvent(std::string_view name, SpanEvent::KeyValue attributes)
    : name{name}, timestamp{std::chrono::system_clock::now()}, attributes{std::move(attributes)} {}

SpanEvent Parse(const formats::json::Value& value, formats::parse::To<SpanEvent>) {
    SpanEvent event{value[kName].As<std::string>()};
    event.timestamp = SpanEvent::Timestamp{std::chrono::nanoseconds{value[kTimeUnixNano].As<std::uint64_t>()}};
    event.attributes = value[kAttributes].As<SpanEvent::KeyValue>({});

    return event;
}

void WriteToStream(const SpanEvent& span_event, formats::json::StringBuilder& sw) {
    const formats::json::StringBuilder::ObjectGuard guard{sw};
    sw.Key(kName);
    sw.WriteString(span_event.name);
    sw.Key(kTimeUnixNano);
    sw.WriteInt64(span_event.timestamp.time_since_epoch().count());
    if (!span_event.attributes.empty()) {
        sw.Key(kAttributes);
        WriteToStream(span_event.attributes, sw);
    }
}

}  // namespace tracing

USERVER_NAMESPACE_END
