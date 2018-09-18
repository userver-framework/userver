#pragma once

#include <memory>

#include <opentracing/tracer.h>

#include <tracing/span.hpp>
#include <tracing/tracer_fwd.hpp>

namespace tracing {

class Tracer : public std::enable_shared_from_this<Tracer> {
 public:
  explicit Tracer(std::shared_ptr<opentracing::Tracer> tracer);

  ~Tracer();

  static void SetTracer(TracerPtr tracer);

  static TracerPtr GetTracer();

  Span CreateSpanWithoutParent(const std::string& name);

  Span CreateSpan(const std::string& name, Span& parent);

 private:
  const std::shared_ptr<opentracing::Tracer> tracer_;
};

}  // namespace tracing
