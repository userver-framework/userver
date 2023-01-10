#include <logging/put_data.hpp>
#include <tracing/span_impl.hpp>
#include <userver/tracing/noop.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {

namespace {
const std::string kTraceIdName = "trace_id";
const std::string kSpanIdName = "span_id";
const std::string kParentIdName = "parent_id";
}  // namespace

class NoopTracer final : public Tracer {
 public:
  NoopTracer(const std::string& service_name) : Tracer(service_name) {}
  void LogSpanContextTo(const Span::Impl&, logging::LogHelper&) const override;
  void LogSpanContextTo(Span::Impl&&, logging::LogHelper&) const override;

 private:
};

void NoopTracer::LogSpanContextTo(Span::Impl&& span,
                                  logging::LogHelper& log_helper) const {
  LogSpanContextTo(span, log_helper);
}

void NoopTracer::LogSpanContextTo(const Span::Impl& span,
                                  logging::LogHelper& log_helper) const {
  PutData(log_helper, kTraceIdName, span.GetTraceId());
  PutData(log_helper, kSpanIdName, span.GetSpanId());
  PutData(log_helper, kParentIdName, span.GetParentId());
}

tracing::TracerPtr MakeNoopTracer(const std::string& service_name) {
  return std::make_shared<tracing::NoopTracer>(service_name);
}

}  // namespace tracing

USERVER_NAMESPACE_END
