#include "span_impl.hpp"

#include <boost/container/small_vector.hpp>

#include <logging/put_data.hpp>
#include <userver/formats/json.hpp>
#include <userver/formats/json/string_builder.hpp>
#include <userver/logging/log_extra.hpp>
#include <userver/tracing/opentracing.hpp>
#include <userver/tracing/tags.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {
namespace {
namespace jaeger {
struct OpentracingTag {
  std::string opentracing_name;
  std::string type;
};

const std::unordered_map<std::string, OpentracingTag>& GetOpentracingTags() {
  static const std::unordered_map<std::string, OpentracingTag> opentracing_tags{
      {kHttpStatusCode, {"http.status_code", "int64"}},
      {kErrorFlag, {"error", "bool"}},
      {kHttpMethod, {"http.method", "string"}},
      {kHttpUrl, {"http.url", "string"}},

      {kDatabaseType, {"db.type", "string"}},
      {kDatabaseStatement, {"db.statement", "string"}},
      {kDatabaseInstance, {"db.instance", "string"}},
      {kDatabaseStatementName, {"db.statement_name", "string"}},
      {kDatabaseCollection, {"db.collection", "string"}},
      {kDatabaseStatementDescription, {"db.query_description", "string"}},

      {kPeerAddress, {"peer.address", "string"}},
  };

  return opentracing_tags;
}

struct LogExtraValueVisitor {
  std::string string_value;

  void operator()(const std::string& val) { string_value = val; }

  void operator()(int val) { string_value = std::to_string(val); }
};

void GetTagObject(formats::json::StringBuilder& builder, const std::string& key,
                  const logging::LogExtra::Value& value,
                  const std::string& type) {
  formats::json::StringBuilder::ObjectGuard guard(builder);
  LogExtraValueVisitor visitor;
  std::visit(visitor, value);

  builder.Key("value");
  builder.WriteString(visitor.string_value);

  builder.Key("type");
  builder.WriteString(type);

  builder.Key("key");
  builder.WriteString(key);
}

const std::string kOperationName = "operation_name";
const std::string kTraceId = "trace_id";
const std::string kParentId = "parent_id";
const std::string kSpanId = "span_id";
const std::string kServiceName = "service_name";

const std::string kStartTime = "start_time";
const std::string kStartTimeMillis = "start_time_millis";
const std::string kDuration = "duration";
}  // namespace jaeger
}  // namespace

void Span::Impl::LogOpenTracing() const {
  auto logger = tracing::OpentracingLogger();
  if (!logger) {
    return;
  }
  DoLogOpenTracing(DO_LOG_TO_NO_SPAN(*logger, log_level_));
}

void Span::Impl::DoLogOpenTracing(logging::LogHelper& lh) const {
  const auto steady_now = std::chrono::steady_clock::now();
  const auto duration = steady_now - start_steady_time_;
  const auto duration_microseconds =
      std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
  auto start_time = std::chrono::duration_cast<std::chrono::microseconds>(
                        start_system_time_.time_since_epoch())
                        .count();

  if (tracer_) {
    PutData(lh, jaeger::kServiceName, tracer_->GetServiceName());
  }
  PutData(lh, jaeger::kTraceId, trace_id_);
  PutData(lh, jaeger::kParentId, parent_id_);
  PutData(lh, jaeger::kSpanId, span_id_);
  PutData(lh, jaeger::kStartTime, start_time);
  PutData(lh, jaeger::kStartTimeMillis, start_time / 1000);
  PutData(lh, jaeger::kDuration, duration_microseconds);
  PutData(lh, jaeger::kOperationName, name_);

  formats::json::StringBuilder tags;
  {
    formats::json::StringBuilder::ArrayGuard guard(tags);
    AddOpentracingTags(tags, log_extra_inheritable_);
    if (log_extra_local_) {
      AddOpentracingTags(tags, *log_extra_local_);
    }
  }
  PutData(lh, "tags", tags.GetStringView());
}

void Span::Impl::AddOpentracingTags(formats::json::StringBuilder& output,
                                    const logging::LogExtra& input) {
  const auto& opentracing_tags = jaeger::GetOpentracingTags();
  for (const auto& [key, value] : *input.extra_) {
    const auto tag_it = opentracing_tags.find(key);
    if (tag_it != opentracing_tags.end()) {
      const auto& tag = tag_it->second;
      jaeger::GetTagObject(output, tag.opentracing_name, value.GetValue(),
                           tag.type);
    }
  }
}

}  // namespace tracing

USERVER_NAMESPACE_END
