#pragma once
#include <userver/tracing/tracer.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {

TracerPtr MakeNoopTracer(const std::string& service_name);

}  // namespace tracing

USERVER_NAMESPACE_END
