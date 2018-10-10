#include <tracing/noop.hpp>

#include <opentracing/noop.h>

namespace tracing {

class NoopTracer : public Tracer {
 public:
  NoopTracer(std::shared_ptr<opentracing::Tracer> tracer) : Tracer(tracer) {}

  virtual void LogSpanContextTo(const opentracing::Span&,
                                logging::LogHelper&) const override {}
};

tracing::TracerPtr MakeNoopTracer() {
  auto ot_tracer = opentracing::MakeNoopTracer();
  return std::make_shared<tracing::NoopTracer>(std::move(ot_tracer));
}

}  // namespace tracing
