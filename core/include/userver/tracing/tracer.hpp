#pragma once

#include <memory>

#include <userver/tracing/span.hpp>
#include <userver/tracing/tracer_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {
class TagWriter;
}  // namespace logging::impl

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

    Span CreateSpan(std::string name, const Span& parent, ReferenceType reference_type);

    // Log tag-private information like trace id, span id, etc.
    virtual void LogSpanContextTo(const Span::Impl& span, logging::impl::TagWriter writer) const = 0;

    logging::LoggerPtr GetOptionalLogger() const { return optional_logger_; }

protected:
    explicit Tracer(std::string_view service_name, logging::LoggerPtr optional_logger)
        : service_name_(service_name), optional_logger_(std::move(optional_logger)) {}

    virtual ~Tracer();

private:
    const std::string service_name_;
    const logging::LoggerPtr optional_logger_;
};

/// Make a tracer that could be set globally via tracing::Tracer::SetTracer
TracerPtr MakeTracer(std::string_view service_name, logging::LoggerPtr logger, std::string_view tracer_type = "native");

}  // namespace tracing

USERVER_NAMESPACE_END
