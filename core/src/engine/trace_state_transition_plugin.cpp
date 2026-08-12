#include <engine/trace_state_transition_plugin.hpp>

#include <chrono>

#include <fmt/chrono.h>

#include <userver/logging/log.hpp>

#include <engine/task/task_context.hpp>
#include <engine/task/task_processor.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

namespace {

void TraceStateTransition(impl::TaskContext& task, TaskBase::State state) noexcept {
    auto& data = task.GetTraceStateTransitionData();

    const bool is_first_run = data.last_state_change_timepoint == std::chrono::steady_clock::time_point{};
    if (is_first_run) {
        data.trace_csw_left = task.GetTaskProcessor().GetTaskTraceMaxCswForNewTask();
    }

    if (data.trace_csw_left == 0) {
        return;
    }
    --data.trace_csw_left;

    auto now = std::chrono::steady_clock::now();
    auto diff = is_first_run ? std::chrono::steady_clock::duration{} : now - data.last_state_change_timepoint;
    auto diff_us = std::chrono::duration_cast<std::chrono::microseconds>(diff);
    data.last_state_change_timepoint = now;

    auto logger = task.GetTaskProcessor().GetTaskTraceLogger();
    if (!logger) {
        return;
    }

    LOG_INFO_TO(
        *logger,
        "Task {:x} changed state to {}, delay = {}",
        task.GetTaskId(),
        TaskBase::GetStateName(state),
        diff_us
    ) << logging::LogExtra::Stacktrace(*logger);
}

}  // namespace

void TraceStateTransitionPlugin::HookTaskStart(impl::TaskContext& task) noexcept {
    TraceStateTransition(task, TaskBase::State::kRunning);
}

void TraceStateTransitionPlugin::HookAfterWakeup(impl::TaskContext& task) noexcept {
    TraceStateTransition(task, TaskBase::State::kRunning);
}

void TraceStateTransitionPlugin::HookBeforeSleep(impl::TaskContext& task) noexcept {
    TraceStateTransition(task, TaskBase::State::kSuspended);
}

void TraceStateTransitionPlugin::HookTaskStop(impl::TaskContext& task) noexcept {
    TraceStateTransition(task, task.GetPendingFinalState());
}

}  // namespace engine

USERVER_NAMESPACE_END
