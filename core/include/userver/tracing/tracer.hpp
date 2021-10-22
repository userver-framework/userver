#pragma once

#include <memory>
#include <unordered_set>

#include <userver/tracing/span.hpp>
#include <userver/tracing/tracer_fwd.hpp>

USERVER_NAMESPACE_BEGIN

/// Opentracing support
namespace tracing {

struct NoLogSpans;

class Tracer : public std::enable_shared_from_this<Tracer> {
 public:
  static void SetNoLogSpans(NoLogSpans&& spans);
  static bool IsNoLogSpan(const std::string& name);

  static void SetTracer(TracerPtr tracer);

  static TracerPtr GetTracer();

  const std::string& GetServiceName() const;

  Span CreateSpanWithoutParent(std::string name);

  Span CreateSpan(std::string name, const Span& parent,
                  ReferenceType reference_type);

  // Log tag-private information like trace id, span id, etc.
  virtual void LogSpanContextTo(const Span::Impl& span,
                                logging::LogHelper& log_helper) const = 0;

  virtual void LogSpanContextTo(Span::Impl&& span,
                                logging::LogHelper& log_helper) const {
    LogSpanContextTo(span, log_helper);
  }

 protected:
  explicit Tracer(std::string_view service_name)
      : service_name_(service_name) {}

  virtual ~Tracer();

 private:
  const std::string service_name_;
};

}  // namespace tracing

USERVER_NAMESPACE_END
