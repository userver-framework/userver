#include <engine/run_in_coro.hpp>
#include <engine/task/task_context.hpp>
#include <rcu/rcu.hpp>
#include <tracing/opentracing.hpp>

namespace tracing {
namespace {
auto& OpentracingLoggerInternal() {
  static rcu::Variable<logging::LoggerPtr> opentracing_logger_ptr;
  return opentracing_logger_ptr;
}

auto& OpentracingLoggerSet() {
  static std::atomic<bool> opentracing_set(false);
  return opentracing_set;
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
  if (!logger) {
    // prevent writing logs into nullptr
    OpentracingLoggerSet() = false;
  }
  OpentracingLoggerInternal().Assign(logger);

  if (logger) {
    OpentracingLoggerSet() = true;
  }
}

bool IsOpentracingLoggerActivated() { return OpentracingLoggerSet(); }

}  // namespace tracing
