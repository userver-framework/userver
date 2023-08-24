#include <userver/tracing/opentracing.hpp>

#include <userver/rcu/rcu.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {
namespace {
auto& OpentracingLoggerInternal() {
  static rcu::Variable<logging::LoggerPtr> opentracing_logger_ptr;
  return opentracing_logger_ptr;
}
}  // namespace

logging::LoggerPtr OpentracingLogger() {
  return OpentracingLoggerInternal().ReadCopy();
}

void SetOpentracingLogger(logging::LoggerPtr logger) {
  UASSERT(engine::current_task::IsTaskProcessorThread());
  OpentracingLoggerInternal().Assign(logger);
}

}  // namespace tracing

USERVER_NAMESPACE_END
