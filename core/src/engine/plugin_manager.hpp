#pragma once

#include <atomic>

#include <boost/range/adaptor/reversed.hpp>

#include <userver/concurrent/variable.hpp>

#include <engine/plugin.hpp>
#include <engine/task/task_processor_mutex_set.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

class PluginManager final {
public:
    PluginManager(TaskProcessor& tp, std::size_t worker_count)
        : mutex_set_(tp, worker_count)
    {}

    ~PluginManager()
    {
        // Wait until all Hook*() are finished
        (void)mutex_set_.WriteLock();
    }

    void RegisterPlugin(PluginBase& plugin) {
        has_any_plugin_ = true;

        auto lock = mutex_set_.WriteLock();
        plugins_.push_back(&plugin);
    }

    void UnregisterPlugin(PluginBase& plugin) noexcept {
        bool empty{false};

        {
            auto lock = mutex_set_.WriteLock();

            auto it = std::find(plugins_.begin(), plugins_.end(), &plugin);
            if (it != plugins_.end()) {
                plugins_.erase(it);
            }

            empty = plugins_.empty();
        }

        if (!empty) {
            has_any_plugin_ = true;
        }
    }

    void HookTaskCreate(const impl::TaskContext& task) {
        if (!has_any_plugin_) {
            return;
        }

        // TaskContext() can be called outside of coroutine.
        // Fallback to "lock all mutexes" if so.
        if (mutex_set_.MayReadLock()) {
            auto lock = mutex_set_.ReadLockFromCoroutine();
            DoHookTaskCreate(task);
        } else {
            auto lock = mutex_set_.WriteLock();
            DoHookTaskCreate(task);
        }
    }

    void HookTaskDestroy(const impl::TaskContext& task) {
        if (!has_any_plugin_) {
            return;
        }

        // ~TaskContext() can be called outside of coroutine.
        // Fallback to "lock all mutexes" if so.
        if (mutex_set_.MayReadLock()) {
            auto lock = mutex_set_.ReadLockFromCoroutine();
            DoHookTaskDestroy(task);
        } else {
            auto lock = mutex_set_.WriteLock();
            DoHookTaskDestroy(task);
        }
    }

    void HookBeforeSleep(const impl::TaskContext& task) {
        if (!has_any_plugin_) {
            return;
        }

        auto lock = mutex_set_.ReadLockFromCoroutine();
        for (auto* const plugin : plugins_) {
            plugin->HookBeforeSleep(task);
        }
    }

    void HookAfterWakeup(const impl::TaskContext& task) {
        if (!has_any_plugin_) {
            return;
        }

        auto lock = mutex_set_.ReadLockFromCoroutine();
        for (auto* const plugin : plugins_ | boost::adaptors::reversed) {
            plugin->HookAfterWakeup(task);
        }
    }

private:
    void DoHookTaskDestroy(const impl::TaskContext& task) {
        for (auto* const plugin : plugins_ | boost::adaptors::reversed) {
            plugin->HookTaskDestroy(task);
        }
    }

    void DoHookTaskCreate(const impl::TaskContext& task) {
        for (auto* const plugin : plugins_) {
            plugin->HookTaskCreate(task);
        }
    }

    std::atomic<bool> has_any_plugin_{false};

    // mutex_set_ protects plugins_ from parallel access from multiple threads.
    // It is a sort of RW mutex. mutex_set_ is bound to a TaskProcessor.
    // If accessed from a task processor worker thread, it can be locked for read very quickly using
    // thread_local std::mutex. Different worker threads don't conflict on read.
    // However, if anyone wants to get a write lock, he has to lock ALL std::Mutex'es belonging to every worker thread.
    // The same for obtaining read lock from non-task processor thread: any worker thread may try to get a read lock, so
    // you have to lock every mutex to prevent it.
    TaskProcessorMutexSet mutex_set_;
    std::vector<PluginBase*> plugins_;
};

}  // namespace engine

USERVER_NAMESPACE_END
