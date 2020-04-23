#pragma once

#include <memory>

#include <tracing/span.hpp>
#include <tracing/tracer_fwd.hpp>

namespace tracing {

class Tracer : public std::enable_shared_from_this<Tracer> {
 public:
  static void SetTracer(TracerPtr tracer);

  static TracerPtr GetTracer();

  const std::string& GetServiceName() const;

  Span CreateSpanWithoutParent(const std::string& name);

  Span CreateSpan(const std::string& name, const Span& parent,
                  ReferenceType reference_type);

  // Log tag-private information like trace id, span id, etc.
  virtual void LogSpanContextTo(const Span::Impl& span,
                                logging::LogHelper& log_helper) const = 0;

  virtual void LogSpanContextTo(Span::Impl&& span,
                                logging::LogHelper& log_helper) const {
    LogSpanContextTo(span, log_helper);
  }

 protected:
  explicit Tracer(const std::string& service_name)
      : service_name_(service_name) {}

  virtual ~Tracer();

 private:
  const std::string service_name_;
};

}  // namespace tracing
