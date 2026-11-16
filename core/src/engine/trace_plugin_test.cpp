#include <engine/tracer_plugin.hpp>

#include <algorithm>

#include <userver/engine/async.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/current_task.hpp>
#include <userver/engine/task/local_variable.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/fast_scope_guard.hpp>

#include <engine/task/task_context.hpp>
#include <engine/task/task_processor.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

struct YieldOnDestruction final {
    ~YieldOnDestruction() { engine::Yield(); }
};

engine::TaskLocalVariable<YieldOnDestruction> kYieldOnDestruction;

}  // namespace

UTEST(TracePlugin, NoRegistrationDuringTaskLocalDestruction) {
    auto& task_processor = engine::current_task::GetTaskProcessor();
    engine::TracePlugin plugin(task_processor.GetWorkerCount());
    task_processor.RegisterPlugin(plugin);
    const utils::FastScopeGuard unregister{[&]() noexcept { task_processor.UnregisterPlugin(plugin); }};

    const engine::impl::TaskContext* context = nullptr;
    const auto count_records = [&plugin, &context] {
        return std::ranges::count(plugin.GetAllTasks(), context, &engine::TracePlugin::Task::task);
    };
    auto task = engine::AsyncNoTracing([&context, &count_records] {
        context = &engine::current_task::GetCurrentTaskContext();
        kYieldOnDestruction.GetOrEmplace();
        engine::Yield();
        EXPECT_EQ(count_records(), 1);
    });
    task.Wait();
    EXPECT_EQ(count_records(), 0);
    task.Get();
    EXPECT_EQ(count_records(), 0);
}

USERVER_NAMESPACE_END
