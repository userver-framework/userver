#include <engine/profiler_execution_plugin.hpp>

#include <chrono>

#include <fmt/chrono.h>

#include <logging/log_extra_stacktrace.hpp>
#include <userver/logging/log.hpp>

#include <engine/task/task_context.hpp>
#include <engine/task/task_processor.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

namespace {

void ProfilerStartExecution(impl::TaskContext& task) noexcept {
    auto& data = task.GetProfilerExecutionData();
    auto threshold_us = task.GetTaskProcessor().GetProfilerThreshold();
    if (threshold_us.count() > 0) {
        data.execute_started = std::chrono::steady_clock::now();
    } else {
        data.execute_started = {};
    }
}

void ProfilerStopExecution(impl::TaskContext& task) noexcept {
    auto& data = task.GetProfilerExecutionData();
    auto& task_processor = task.GetTaskProcessor();
    auto threshold_us = task_processor.GetProfilerThreshold();
    if (threshold_us.count() <= 0) {
        return;
    }

    if (data.execute_started == std::chrono::steady_clock::time_point{}) {
        // the task was started w/o profiling, skip it
        return;
    }

    auto now = std::chrono::steady_clock::now();
    auto duration = now - data.execute_started;
    auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(duration);

    if (duration_us >= threshold_us) {
        logging::LogExtra extra_stacktrace;
        if (task_processor.ShouldProfilerForceStacktrace()) {
            logging::impl::ExtendLogExtraWithStacktrace(extra_stacktrace);
        }
        LOG_ERROR(
            "Profiler threshold reached, task was executing for too long without context switch ({} >= {})",
            duration_us,
            threshold_us
        ) << extra_stacktrace;
    }
}

}  // namespace

void ProfilerExecutionPlugin::HookTaskStart(impl::TaskContext& task) noexcept { ProfilerStartExecution(task); }

void ProfilerExecutionPlugin::HookAfterWakeup(impl::TaskContext& task) noexcept { ProfilerStartExecution(task); }

void ProfilerExecutionPlugin::HookBeforeSleep(impl::TaskContext& task) noexcept { ProfilerStopExecution(task); }

void ProfilerExecutionPlugin::HookTaskStop(impl::TaskContext& task) noexcept { ProfilerStopExecution(task); }

}  // namespace engine

USERVER_NAMESPACE_END
