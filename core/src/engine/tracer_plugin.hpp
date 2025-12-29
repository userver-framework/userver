#pragma once

#include <unordered_map>
#include <unordered_set>
#include <utility>

#include <boost/stacktrace/stacktrace.hpp>

#include <userver/concurrent/variable.hpp>
#include <userver/logging/log.hpp>
#include <userver/logging/stacktrace_cache.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/algo.hpp>

#include <engine/plugin.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

namespace impl {
class TaskContext;
}

class TracePlugin final : public PluginBase {
public:
    explicit TracePlugin(std::size_t worker_count);

    void HookTaskCreate(const impl::TaskContext&) noexcept override;

    void HookTaskDestroy(const impl::TaskContext& task) noexcept override;

    void HookBeforeSleep(const impl::TaskContext& task) noexcept override;

    void HookAfterWakeup(const impl::TaskContext&) noexcept override;

    void PrintStacksByComponentNames(const std::unordered_set<std::string>& component_names) const;

    struct Task {
        const impl::TaskContext* task;
        const std::string component_name;
        const std::string span_name;
        const std::string link;
        boost::stacktrace::stacktrace last_bt_before_sleep;
    };

    std::vector<Task> GetAllTasks() const;

    void Clear();

private:
    void PrintTask(const impl::TaskContext* context, const Task& task) const;

    using TaskMap = std::unordered_map<const impl::TaskContext*, Task>;

    concurrent::LockedPtr<std::lock_guard<std::mutex>, TaskMap> LockForTask(const impl::TaskContext& task);

    // alive_tasks_ is a TaskMap sharded into task processor worker threads count shards.
    // It allows to reduce contention on std::mutex on average.
    std::vector<concurrent::Variable<TaskMap, std::mutex>> alive_tasks_;
};

}  // namespace engine

USERVER_NAMESPACE_END
