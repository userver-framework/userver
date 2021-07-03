#pragma once
#include <tracing/tracer.hpp>

namespace tracing {

TracerPtr MakeNoopTracer(const std::string& service_name);

}  // namespace tracing
