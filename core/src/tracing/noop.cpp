#include <userver/tracing/noop.hpp>

#include <tracing/span_impl.hpp>
#include <userver/logging/impl/tag_writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {

namespace {
constexpr std::string_view kTraceIdName = "trace_id";
constexpr std::string_view kSpanIdName = "span_id";
constexpr std::string_view kParentIdName = "parent_id";
}  // namespace

class NoopTracer final : public Tracer {
 public:
  NoopTracer(const std::string& service_name) : Tracer(service_name) {}

  void LogSpanContextTo(const Span::Impl&,
                        logging::impl::TagWriter) const override;

 private:
};

void NoopTracer::LogSpanContextTo(const Span::Impl& span,
                                  logging::impl::TagWriter writer) const {
  writer.PutTag(kTraceIdName, span.GetTraceId());
  writer.PutTag(kSpanIdName, span.GetSpanId());
  writer.PutTag(kParentIdName, span.GetParentId());
}

tracing::TracerPtr MakeNoopTracer(const std::string& service_name) {
  return std::make_shared<tracing::NoopTracer>(service_name);
}

}  // namespace tracing

USERVER_NAMESPACE_END
