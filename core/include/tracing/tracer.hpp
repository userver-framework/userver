#pragma once

#include <memory>

#include <tracing/span.hpp>
#include <tracing/tracer_fwd.hpp>

namespace tracing {

class Tracer : public std::enable_shared_from_this<Tracer> {
 public:
  static void SetTracer(TracerPtr tracer);

  static TracerPtr GetTracer();

  Span CreateSpanWithoutParent(const std::string& name);

  Span CreateSpan(const std::string& name, const Span& parent,
                  ReferenceType reference_type);

  // Log tag-private information like trace id, span id, etc.
  virtual void LogSpanContextTo(const Span::Impl& span,
                                logging::LogHelper& log_helper) const = 0;

 protected:
  virtual ~Tracer();
};

}  // namespace tracing
