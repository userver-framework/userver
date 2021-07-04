#include <engine/task/task_context.hpp>
#include <userver/engine/run_in_coro.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/tracing/opentracing.hpp>

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
  if (engine::current_task::GetCurrentTaskContextUnchecked() == nullptr) {
    RunInCoro([&logger] { SetOpentracingLogger(logger); }, 1);
    return;
  }

  OpentracingLoggerInternal().Assign(logger);
}

}  // namespace tracing
