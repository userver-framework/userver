#pragma once

#include <opentracing/value.h>
#include <logging/log_extra.hpp>

namespace tracing {

opentracing::Value ToOpentracingValue(logging::LogExtra::Value);

logging::LogExtra::Value FromOpentracingValue(opentracing::Value);

}  // namespace tracing
