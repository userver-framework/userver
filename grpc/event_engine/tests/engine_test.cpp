#include <ugrpc/impl/event_engine/engine.hpp>

#include "test_utils.hpp"

#if defined(ARCADIA_ROOT) && defined(__linux__)

#include <atomic>
#include <chrono>
#include <memory>
#include <optional>
#include <utility>

#include <userver/compiler/thread_local.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/multi_consumer_event.hpp>
#include <userver/engine/single_use_event.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/current_task.hpp>
#include <userver/engine/task/local_variable.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/fast_scope_guard.hpp>

#include <engine/ev/thread_pool.hpp>
#include <engine/task/task_processor.hpp>

#include <ugrpc/impl/event_engine/executor.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl::event_engine {
namespace {

struct TaskCleanupState {
    engine::SingleUseEvent entered;
    engine::SingleUseEvent resume;
    bool finished{false};
};

class PausedTaskCleanup final {
public:
    explicit PausedTaskCleanup(std::shared_ptr<TaskCleanupState> state)
        : state_(std::move(state))
    {}

    ~PausedTaskCleanup() {
        state_->entered.Send();
        state_->resume.WaitNonCancellable();
        state_->finished = true;
    }

private:
    std::shared_ptr<TaskCleanupState> state_;
};

engine::TaskLocalVariable<PausedTaskCleanup> paused_task_cleanup;

}  // namespace

UTEST(GrpcServerEventEngine, StoppedEngineCanOutliveOwnerAndBeDestroyedOnEvThread) {
    auto& task_processor = engine::current_task::GetTaskProcessor();
    std::shared_ptr<PosixEventEngine> external;
    bool callback_finished = false;
    {
        PosixEventEngineOwner owner{task_processor};
        external = GetPosixEventEngine(owner);
        external->Run([&] { callback_finished = true; });
        external->RunAfter(std::chrono::hours{1}, [] { ADD_FAILURE() << "Cancelled timer ran"; });
    }
    EXPECT_TRUE(callback_finished);
    EXPECT_TRUE(external->IsStopped());
    EXPECT_EQ(&external->GetTaskProcessor(), &task_processor);
    const std::weak_ptr<PosixEventEngine> weak = external;
    task_processor.EventThreadPool().NextThread().RunInEvLoopSync([external = std::move(external)]() mutable {
        EXPECT_FALSE(engine::current_task::IsTaskProcessorThread());
        external.reset();
    });
    EXPECT_TRUE(weak.expired());
}

UTEST(GrpcServerEventEngine, ShutdownQuiescesTasksHoldingTheEngine) {
    std::atomic<bool> callback_finished{false};
    std::optional<PosixEventEngineOwner> owner{std::in_place, engine::current_task::GetTaskProcessor()};
    const std::weak_ptr<PosixEventEngine> weak = GetPosixEventEngine(*owner);
    {
        const compiler::CoroutineSwitchBanScope submission;
        auto engine = GetPosixEventEngine(*owner);
        engine->Run([engine, &callback_finished] { callback_finished.store(true); });
    }
    owner.reset();
    EXPECT_TRUE(callback_finished.load());
    EXPECT_TRUE(weak.expired());
}

UTEST_MT(GrpcServerEventEngine, PostedTasksCanPostDescendantsDuringQuiesce, 4) {
    PosixEventEngineOwner owner{engine::current_task::GetTaskProcessor()};
    engine::MultiConsumerEvent release;
    std::atomic<int> finished{0};
    constexpr int kParents = 4;
    constexpr int kChildren = 32;
    const auto engine = GetPosixEventEngine(owner);
    for (int parent = 0; parent < kParents; ++parent) {
        engine->Post([&, engine] {
            release.Wait();
            EXPECT_TRUE(engine->IsStopping());
            for (int child = 0; child < kChildren; ++child) {
                engine->Post([&, engine] {
                    engine->Run([&, engine] {
                        EXPECT_TRUE(engine->IsStopping());
                        EXPECT_TRUE(Executor::IsInCallback());
                        ++finished;
                    });
                });
            }
        });
    }
    auto shutdown = engine::AsyncNoTracing([&] { owner.Shutdown(); });
    while (!GetPosixEventEngine(owner)->IsStopping()) {
        engine::Yield();
    }
    EXPECT_FALSE(shutdown.IsFinished());
    release.Send();
    shutdown.Get();
    EXPECT_EQ(finished.load(), kParents * kChildren);
}

UTEST(GrpcServerEventEngine, ShutdownWaitsForPostedTaskCleanup) {
    auto state = std::make_shared<TaskCleanupState>();
    PosixEventEngineOwner owner{engine::current_task::GetTaskProcessor()};
    GetPosixEventEngine(owner)->Post([state] {
        Executor::RunCallback([state] { paused_task_cleanup.GetOrEmplace(state); });
        EXPECT_FALSE(Executor::IsInCallback());
        engine::Yield();
    });
    state->entered.WaitNonCancellable();
    engine::SingleUseEvent shutdown_started;
    engine::TaskWithResult<void> shutdown;
    utils::FastScopeGuard resume_on_failure([&]() noexcept { state->resume.Send(); });
    shutdown = engine::AsyncNoTracing([&] {
        shutdown_started.Send();
        owner.Shutdown();
    });
    shutdown_started.WaitNonCancellable();
    EXPECT_FALSE(shutdown.IsFinished());
    EXPECT_FALSE(GetPosixEventEngine(owner)->IsStopped());
    state->resume.Send();
    resume_on_failure.Release();
    shutdown.Get();
    EXPECT_TRUE(state->finished);
    EXPECT_TRUE(GetPosixEventEngine(owner)->IsStopped());
}

}  // namespace ugrpc::impl::event_engine

USERVER_NAMESPACE_END

#endif
