#pragma once

#include <memory>

#include <userver/rcu/fwd.hpp>
#include <userver/tracing/span.hpp>
#include <userver/tracing/tracer_fwd.hpp>

#include <dynamic_config/variables/USERVER_NO_LOG_SPANS.hpp>

USERVER_NAMESPACE_BEGIN

/// Opentracing support
namespace tracing {

using NoLogSpans = ::dynamic_config::userver_no_log_spans::VariableType;

void SetNoLogSpans(NoLogSpans&& spans);
bool IsNoLogSpan(const std::string& name);
NoLogSpans CopyNoLogSpans();

// For legacy opentracing support only.
class Tracer final {
public:
    static void SetTracer(Tracer&& tracer);

    static rcu::ReadablePtr<Tracer, rcu::ExclusiveRcuTraits> ReadTracer();

    static Tracer CopyCurrentTracer();

    Tracer(std::string_view service_name)
        : service_name_(service_name)
    {}

    // Only works if legacy opentracing is set up.
    const std::string& GetServiceName() const;

private:
    std::string service_name_;
};

// For tests and benchmarks only!
class TracerCleanupScope final {
public:
    TracerCleanupScope();

    TracerCleanupScope(TracerCleanupScope&&) = delete;
    TracerCleanupScope& operator=(TracerCleanupScope&&) = delete;
    ~TracerCleanupScope();

private:
    Tracer old_tracer_;
    NoLogSpans old_no_log_spans_;
};

}  // namespace tracing

USERVER_NAMESPACE_END
