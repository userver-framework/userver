#pragma once

#include <memory>

#include <opentracing/tracer.h>

#include <tracing/span.hpp>
#include <tracing/tracer_fwd.hpp>

namespace tracing {

class Tracer : public std::enable_shared_from_this<Tracer> {
 public:
  static void SetTracer(TracerPtr tracer);

  static TracerPtr GetTracer();

  Span CreateSpanWithoutParent(const std::string& name);

  Span CreateSpan(const std::string& name, const Span& parent);

  // Log tag-private information like trace id, span id, etc.
  virtual void LogSpanContextTo(const opentracing::Span& span,
                                logging::LogHelper& log_helper) const = 0;

 protected:
  explicit Tracer(std::shared_ptr<opentracing::Tracer> tracer);

  virtual ~Tracer();

 private:
  const std::shared_ptr<opentracing::Tracer> tracer_;
};

}  // namespace tracing
