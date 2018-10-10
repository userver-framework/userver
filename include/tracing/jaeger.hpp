#pragma once
#include <tracing/tracer.hpp>

namespace tracing {

tracing::TracerPtr MakeJaegerLogTracer();
}
