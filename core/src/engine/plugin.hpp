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

    virtual void HookTaskCreate(const impl::TaskContext& task) noexcept = 0;

    virtual void HookTaskDestroy(const impl::TaskContext& task) noexcept = 0;

    /// Callback that is called in coroutine just before falling asleep
    /// and switching execution to another coroutine.
    virtual void HookBeforeSleep(const impl::TaskContext& task) noexcept = 0;

    /// Callback that is called in coroutine just after wakeup.
    virtual void HookAfterWakeup(const impl::TaskContext& task) noexcept = 0;
};

}  // namespace engine

USERVER_NAMESPACE_END
