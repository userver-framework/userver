#include <userver/tracing/opentracing.hpp>

#include <userver/engine/run_standalone.hpp>
#include <userver/engine/task/task.hpp>
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
  if (!engine::current_task::GetTaskProcessorOptional()) {
    // TODO TAXICOMMON-4233 remove
    engine::RunStandalone([&logger] { SetOpentracingLogger(logger); });
    return;
  }

  OpentracingLoggerInternal().Assign(logger);
}

}  // namespace tracing

USERVER_NAMESPACE_END
