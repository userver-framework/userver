#pragma once
#include <memory>

#include <userver/logging/logger.hpp>

namespace tracing {
/// Returns opentracing logger
logging::LoggerPtr OpentracingLogger();

/// Atomically replaces span logger
void SetOpentracingLogger(logging::LoggerPtr);

}  // namespace tracing
