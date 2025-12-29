#include <engine/tracer_plugin.hpp>

#include <unordered_set>
#include <utility>

#include <boost/stacktrace/stacktrace.hpp>

#include <userver/concurrent/variable.hpp>
#include <userver/logging/log.hpp>
#include <userver/logging/stacktrace_cache.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/algo.hpp>

#include <engine/plugin.hpp>
#include <engine/task/task_context.hpp>
#include <engine/task/task_processor.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

TracePlugin::TracePlugin(std::size_t worker_count)
    : alive_tasks_(worker_count)
{}

void TracePlugin::HookTaskCreate(const impl::TaskContext&) noexcept {}

void TracePlugin::HookTaskDestroy(const impl::TaskContext& task) noexcept {
    auto tasks = LockForTask(task);
    tasks->erase(&task);
}

void TracePlugin::HookBeforeSleep(const impl::TaskContext& task) noexcept {
    // stacktrace is slow, not under lock
    auto bt = boost::stacktrace::stacktrace();

    auto tasks = LockForTask(task);
    auto it = tasks->find(&task);

    if (it != tasks->end()) {
        // fast path
        it->second.last_bt_before_sleep = std::move(bt);
    } else {
        // slow path
        auto* span = tracing::Span::CurrentSpanUnchecked();
        std::string component_name{span ? span->GetTag("component_name") : std::string_view{}};
        std::string span_name{span ? span->GetName() : std::string_view{}};
        std::string link{span ? span->GetLink() : std::string_view{}};

        [[maybe_unused]] auto [it, ok] =
            tasks->emplace(&task, Task{&task, component_name, std::move(span_name), std::move(link), std::move(bt)});
        UASSERT(ok);
    }
}

void TracePlugin::HookAfterWakeup(const impl::TaskContext&) noexcept {}

void TracePlugin::PrintStacksByComponentNames(const std::unordered_set<std::string>& component_names) const {
    auto tasks = GetAllTasks();
    for (const auto& task : tasks) {
        if (!utils::FindOrNullptr(component_names, task.component_name)) {
            continue;
        }

        PrintTask(task.task, task);
    }
}

std::vector<TracePlugin::Task> TracePlugin::GetAllTasks() const {
    std::vector<Task> result;
    result.reserve(alive_tasks_.size());

    for (const auto& map : alive_tasks_) {
        auto tasks = map.Lock();
        for (const auto& task : *tasks) {
            result.push_back(task.second);
        }
    }
    return result;
}

void TracePlugin::Clear() {
    for (auto& map : alive_tasks_) {
        auto tasks = map.Lock();
        tasks->clear();
    }
}

void TracePlugin::PrintTask(const impl::TaskContext* context, const Task& task) const {
    logging::LogExtra log_extra{
        {"span", task.span_name},
        {"link", task.link},
        {"component_name", task.component_name},
    };
    LOG_INFO() << "Task " << context << task.last_bt_before_sleep << log_extra;
}

concurrent::LockedPtr<std::lock_guard<std::mutex>, TracePlugin::TaskMap> TracePlugin::LockForTask(
    const impl::TaskContext& task
) {
    auto hash = std::hash<std::intptr_t>{}(reinterpret_cast<std::intptr_t>(&task));
    auto& variable = alive_tasks_[hash % alive_tasks_.size()];
    return variable.Lock();
}

}  // namespace engine

USERVER_NAMESPACE_END
