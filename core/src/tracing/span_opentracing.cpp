#include "span_impl.hpp"

#include <unordered_map>

#include <boost/container/small_vector.hpp>

#include <userver/formats/json/string_builder.hpp>
#include <userver/logging/impl/tag_writer.hpp>
#include <userver/logging/log_extra.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/tracing/tags.hpp>
#include <userver/utils/trivial_map.hpp>

#include <logging/log_helper_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {

namespace {
namespace jaeger {

struct OpentracingTag {
    std::string_view opentracing_name;
    std::string_view type;
};

struct LogExtraValueVisitor {
    std::string string_value;

    void operator()(const std::string& val) { string_value = val; }

    void operator()(int val) { string_value = std::to_string(val); }

    void operator()(const logging::JsonString& val) { string_value = val.GetView(); }
};

void GetTagObject(
    formats::json::StringBuilder& builder,
    std::string_view key,
    const logging::LogExtra::Value& value,
    std::string_view type
) {
    const formats::json::StringBuilder::ObjectGuard guard(builder);
    LogExtraValueVisitor visitor;
    std::visit(visitor, value);

    builder.Key("value");
    builder.WriteString(visitor.string_value);

    builder.Key("type");
    builder.WriteString(type);

    builder.Key("key");
    builder.WriteString(key);
}

constexpr std::string_view kOperationName = "operation_name";
constexpr std::string_view kTraceId = "trace_id";
constexpr std::string_view kParentId = "parent_id";
constexpr std::string_view kSpanId = "span_id";
constexpr std::string_view kServiceName = "service_name";

constexpr std::string_view kStartTime = "start_time";
constexpr std::string_view kStartTimeMillis = "start_time_millis";
constexpr std::string_view kDuration = "duration";

constexpr std::string_view kTags = "tags";

}  // namespace jaeger

}  // namespace

void Span::Impl::LogOpenTracing() const {
    auto tracer = Tracer::ReadTracer();

    const auto& opentracing_logger = tracer->GetOptionalLogger();
    if (!opentracing_logger) {
        return;
    }

    const impl::DetachLocalSpansScope ignore_local_span;
    logging::LogHelper lh(*opentracing_logger, log_level_, logging::LogClass::kTrace);
    DoLogOpenTracing(*tracer, lh.GetTagWriter());
}

void Span::Impl::DoLogOpenTracing(const Tracer& tracer, logging::impl::TagWriter writer) const {
    const auto steady_now = std::chrono::steady_clock::now();
    const auto duration = steady_now - start_steady_time_;
    const auto duration_microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    auto start_time =
        std::chrono::duration_cast<std::chrono::microseconds>(start_system_time_.time_since_epoch()).count();

    writer.PutTag(jaeger::kServiceName, tracer.GetServiceName());
    writer.PutTag(jaeger::kTraceId, std::string{GetTraceId()});
    writer.PutTag(jaeger::kParentId, std::string{GetParentId()});
    writer.PutTag(jaeger::kSpanId, std::string{GetSpanId()});
    writer.PutTag(jaeger::kStartTime, start_time);
    writer.PutTag(jaeger::kStartTimeMillis, start_time / 1000);
    writer.PutTag(jaeger::kDuration, duration_microseconds);
    writer.PutTag(jaeger::kOperationName, name_);

    formats::json::StringBuilder tags;
    {
        const formats::json::StringBuilder::ArrayGuard guard(tags);
        AddOpentracingTags(tags, log_extra_inheritable_);
        if (log_extra_local_) {
            AddOpentracingTags(tags, *log_extra_local_);
        }
    }
    writer.PutTag(jaeger::kTags, tags.GetStringView());
}

void Span::Impl::AddOpentracingTags(formats::json::StringBuilder& output, const logging::LogExtra& input) {
    using OpentracingTag = jaeger::OpentracingTag;
    static const std::unordered_map<std::string_view, OpentracingTag> kGetOpentracingTags = {
        {kHttpStatusCode, OpentracingTag{"http.status_code", "int64"}},
        {kErrorFlag, OpentracingTag{"error", "bool"}},
        {kHttpMethod, OpentracingTag{"http.method", "string"}},
        {kHttpUrl, OpentracingTag{"http.url", "string"}},

        {kDatabaseType, OpentracingTag{"db.type", "string"}},
        {kDatabaseStatement, OpentracingTag{"db.statement", "string"}},
        {kDatabaseInstance, OpentracingTag{"db.instance", "string"}},
        {kDatabaseStatementName, OpentracingTag{"db.statement_name", "string"}},
        {kDatabaseCollection, OpentracingTag{"db.collection", "string"}},
        {kDatabaseStatementDescription, OpentracingTag{"db.query_description", "string"}},

        {kPeerAddress, OpentracingTag{"peer.address", "string"}},
    };

    for (const auto& [key, value] : *input.extra_) {
        const auto tag_it = kGetOpentracingTags.find(key);
        if (tag_it != kGetOpentracingTags.cend()) {
            const auto& tag = tag_it->second;
            jaeger::GetTagObject(output, tag.opentracing_name, value.GetValue(), tag.type);
        }
    }
}

}  // namespace tracing

USERVER_NAMESPACE_END
