#include <tracing/noop.hpp>
#include <tracing/span_impl.hpp>

namespace tracing {

namespace {
const std::string kTraceIdName = "trace_id";
const std::string kSpanIdName = "span_id";
const std::string kParentIdName = "parent_id";
}  // namespace

class NoopTracer : public Tracer {
 public:
  virtual void LogSpanContextTo(const Span::Impl&,
                                logging::LogHelper&) const override;
};

void NoopTracer::LogSpanContextTo(const Span::Impl& span,
                                  logging::LogHelper& log_helper) const {
  const auto& trace_id_str = span.GetTraceId();
  const auto& span_id_str = span.GetSpanId();
  const auto& parent_id_str = span.GetParentId();

  logging::LogExtra result({{kTraceIdName, trace_id_str},
                            {kSpanIdName, span_id_str},
                            {kParentIdName, parent_id_str}});
  log_helper << std::move(result);
}

tracing::TracerPtr MakeNoopTracer() {
  return std::make_shared<tracing::NoopTracer>();
}

}  // namespace tracing
