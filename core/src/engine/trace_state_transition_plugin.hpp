#pragma once

#include <engine/plugin.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

namespace impl {
class TaskContext;
}

/// Plugin that logs task state transitions to the task-trace logger by driving
/// TaskContext::TraceStateTransition via task hooks. Covers kRunning, kSuspended
/// and the terminal state; kQueued stays in TaskProcessor::Schedule. Has no cost
/// unless registered (i.e. unless task tracing is on).
class TraceStateTransitionPlugin final : public PluginBase {
public:
    void HookTaskStart(impl::TaskContext& task) noexcept override;

    void HookAfterWakeup(impl::TaskContext& task) noexcept override;

    void HookBeforeSleep(impl::TaskContext& task) noexcept override;

    void HookTaskStop(impl::TaskContext& task) noexcept override;
};

}  // namespace engine

USERVER_NAMESPACE_END
