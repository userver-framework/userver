#pragma once

#include <atomic>
#include <shared_mutex>
#include <vector>

#include <userver/concurrent/variable.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

namespace impl {
class TaskContext;
}

/// Base class for engine plugin
class PluginBase {
public:
    virtual ~PluginBase() = default;

    virtual void HookTaskCreate(impl::TaskContext& /*task*/) noexcept {}

    virtual void HookTaskDestroy(impl::TaskContext& /*task*/) noexcept {}

    /// Callback that is called in coroutine just before falling asleep
    /// and switching execution to another coroutine.
    virtual void HookBeforeSleep(impl::TaskContext& /*task*/) noexcept {}

    /// Callback that is called in coroutine just after wakeup.
    virtual void HookAfterWakeup(impl::TaskContext& /*task*/) noexcept {}

    /// Callback that is called in coroutine when the task starts executing its
    /// payload (once per task, before the first user code runs).
    virtual void HookTaskStart(impl::TaskContext& /*task*/) noexcept {}

    /// Callback that is called in coroutine when the task finishes executing its
    /// payload (once per task, after completion or cancellation). The terminal
    /// state is available via task.GetPendingFinalState().
    /// Task-local variables are destroyed after this callback is called.
    /// Note: the task may sleep during the destruction of task-local variables
    /// after this callback is called.
    virtual void HookTaskStop(impl::TaskContext& /*task*/) noexcept {}
};

}  // namespace engine

USERVER_NAMESPACE_END
