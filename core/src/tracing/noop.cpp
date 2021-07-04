#include <tracing/span_impl.hpp>
#include <userver/tracing/noop.hpp>

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
  template <class SpanImpl>
  void LogSpanContextToImpl(SpanImpl&&, logging::LogHelper&) const;
};

void NoopTracer::LogSpanContextTo(const Span::Impl& span,
                                  logging::LogHelper& log_helper) const {
  LogSpanContextToImpl(span, log_helper);
}

void NoopTracer::LogSpanContextTo(Span::Impl&& span,
                                  logging::LogHelper& log_helper) const {
  LogSpanContextToImpl(std::move(span), log_helper);
}

template <class SpanImpl>
void NoopTracer::LogSpanContextToImpl(SpanImpl&& span,
                                      logging::LogHelper& log_helper) const {
  logging::LogExtra result;
  result.Extend(kTraceIdName, std::forward<SpanImpl>(span).GetTraceId());
  result.Extend(kSpanIdName, std::forward<SpanImpl>(span).GetSpanId());
  result.Extend(kParentIdName, std::forward<SpanImpl>(span).GetParentId());

  log_helper << std::move(result);
}

tracing::TracerPtr MakeNoopTracer(const std::string& service_name) {
  return std::make_shared<tracing::NoopTracer>(service_name);
}

}  // namespace tracing
