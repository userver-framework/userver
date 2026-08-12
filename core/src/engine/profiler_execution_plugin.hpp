#pragma once

#include <engine/plugin.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

namespace impl {
class TaskContext;
}

/// Plugin that drives TaskContext's execution profiler (warns when a task runs
/// for too long between context switches) via task hooks. Has no cost unless
/// registered (i.e. unless profiling is enabled).
class ProfilerExecutionPlugin final : public PluginBase {
public:
    void HookTaskStart(impl::TaskContext& task) noexcept override;

    void HookAfterWakeup(impl::TaskContext& task) noexcept override;

    void HookBeforeSleep(impl::TaskContext& task) noexcept override;

    void HookTaskStop(impl::TaskContext& task) noexcept override;
};

}  // namespace engine

USERVER_NAMESPACE_END
