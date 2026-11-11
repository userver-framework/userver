#include <ugrpc/impl/event_engine/timer_manager.hpp>
#include <ugrpc/impl/event_engine/timer_utils.hpp>

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>
#include <vector>

#include <userver/engine/async.hpp>
#include <userver/engine/future.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/engine/single_use_event.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/current_task.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/fast_scope_guard.hpp>

#include <ugrpc/impl/event_engine/executor.hpp>

#include "test_utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl::event_engine {
namespace {

using namespace std::chrono_literals;

UTEST(GrpcEventEngineTimers, NonpositiveDelaysAreDeferredAndCannotBeCancelled) {
    PosixEventEngineOwner owner{engine::current_task::GetTaskProcessor()};
    auto ee = owner.GetEventEngine();
    int calls = 0;
    for (const auto delay : {0ns, -1ns}) {
        const auto handle = ee->RunAfter(delay, [&] {
            EXPECT_TRUE(Executor::IsInCallback());
            ++calls;
        });
        EXPECT_EQ(handle, grpc_event_engine::experimental::EventEngine::TaskHandle::kInvalid);
        EXPECT_FALSE(ee->Cancel(handle));
    }
    EXPECT_EQ(calls, 0);
    owner.Shutdown();
    EXPECT_EQ(calls, 2);
}

UTEST(GrpcEventEngineTimers, PositiveDelayDoesNotFireEarlyAndRunsOnTheTargetProcessor) {
    auto& tp = engine::current_task::GetTaskProcessor();
    PosixEventEngineOwner owner{tp};
    engine::Promise<TimerClock::time_point> fired;
    auto future = fired.get_future();
    const auto before = TimerClock::now();
    const auto handle = owner.GetEventEngine()->RunAfter(20ms, [&] {
        EXPECT_EQ(&engine::current_task::GetTaskProcessor(), &tp);
        EXPECT_TRUE(Executor::IsInCallback());
        fired.set_value(TimerClock::now());
    });
    ASSERT_EQ(future.wait_for(utest::kMaxTestWaitTime), engine::FutureStatus::kReady);
    EXPECT_GE(future.get(), before + 20ms);
    EXPECT_FALSE(owner.GetEventEngine()->Cancel(handle));
}

UTEST(GrpcEventEngineTimers, CancelDestroysCapturesInScopeAndRejectsAStaleHandle) {
    PosixEventEngineOwner owner{engine::current_task::GetTaskProcessor()};
    auto ee = owner.GetEventEngine();
    int destroyed = 0;
    const auto handle =
        ee->RunAfter(1h, [capture = utils::FastScopeGuard{[&]() noexcept {
                              EXPECT_TRUE(Executor::IsInCallback());
                              ++destroyed;
                          }}] { ADD_FAILURE() << "Cancelled timer ran"; });
    EXPECT_TRUE(ee->Cancel(handle));
    EXPECT_EQ(destroyed, 1);
    EXPECT_FALSE(ee->Cancel(handle));
    const auto replacement = ee->RunAfter(1h, [] { ADD_FAILURE() << "Pending replacement ran"; });
    EXPECT_FALSE(ee->Cancel(handle));
    EXPECT_TRUE(ee->Cancel(replacement));
}

TEST(GrpcEventEngineTimers, ShutdownCancelsDeferredRegistrationsOnEveryEvThread) {
    engine::TaskProcessorPoolsConfig config;
    config.ev_threads_num = 2;
    engine::RunStandalone(1, config, [] {
        PosixEventEngineOwner owner{engine::current_task::GetTaskProcessor()};
        int destroyed = 0;
        for (int i = 0; i < 256; ++i) {
            std::ignore =
                owner.GetEventEngine()
                    ->RunAfter(1h, [capture = utils::FastScopeGuard{[&]() noexcept {
                                        EXPECT_TRUE(Executor::IsInCallback());
                                        ++destroyed;
                                    }}] { ADD_FAILURE() << "Pending timer ran"; });
        }
        owner.Shutdown();
        EXPECT_EQ(destroyed, 256);
    });
}

UTEST(GrpcEventEngineTimers, ShutdownRejectsNewDelayedTimersFromCaptureDestruction) {
    PosixEventEngineOwner owner{engine::current_task::GetTaskProcessor()};
    auto ee = owner.GetEventEngine();
    int destroyed = 0;
    std::ignore =
        ee->RunAfter(1h, [capture = utils::FastScopeGuard{[&]() noexcept {
                              const auto late =
                                  ee->RunAfter(1h, [nested = utils::FastScopeGuard{[&]() noexcept {
                                                        EXPECT_TRUE(Executor::IsInCallback());
                                                        ++destroyed;
                                                    }}] { ADD_FAILURE() << "Late timer ran"; });
                              EXPECT_EQ(late, grpc_event_engine::experimental::EventEngine::TaskHandle::kInvalid);
                          }}] { ADD_FAILURE() << "Pending timer ran"; });
    owner.Shutdown();
    EXPECT_EQ(destroyed, 1);
}

UTEST_MT(GrpcEventEngineTimers, CancellationRacesWithExpiryExactlyOnce, 4) {
    struct Counters {
        std::atomic<int> calls{0};
        std::atomic<int> destroyed{0};
        bool cancelled{};
    };
    std::vector<std::shared_ptr<Counters>> counts;
    PosixEventEngineOwner owner{engine::current_task::GetTaskProcessor()};
    auto ee = owner.GetEventEngine();
    std::vector<grpc_event_engine::experimental::EventEngine::TaskHandle> handles;
    for (int i = 0; i < 256; ++i) {
        auto counters = std::make_shared<Counters>();
        counts.push_back(counters);
        handles.push_back(ee->RunAfter(1ms, [counters, capture = utils::FastScopeGuard{[counters]() noexcept {
                                                           ++counters->destroyed;
                                                       }}] { ++counters->calls; }));
    }
    std::jthread cancel([&] {
        for (std::size_t i = 0; i < handles.size(); ++i) {
            counts[i]->cancelled = ee->Cancel(handles[i]);
        }
    });
    cancel.join();
    owner.Shutdown();
    for (const auto& counters : counts) {
        EXPECT_EQ(counters->calls, counters->cancelled ? 0 : 1);
        EXPECT_EQ(counters->destroyed, 1);
    }
}

UTEST(GrpcEventEngineTimers, ShutdownWaitsForForeignCancellationWithoutBlockingTheOnlyWorker) {
    engine::SingleUseEvent entered;
    std::atomic<bool> release{false};
    PosixEventEngineOwner owner{engine::current_task::GetTaskProcessor()};
    auto ee = owner.GetEventEngine();
    const auto handle =
        ee->RunAfter(1h, [capture = utils::FastScopeGuard{[&]() noexcept {
                              EXPECT_FALSE(engine::current_task::IsTaskProcessorThread());
                              EXPECT_TRUE(Executor::IsInCallback());
                              entered.Send();
                              release.wait(false);
                          }}] { ADD_FAILURE() << "Cancelled timer ran"; });
    std::jthread cancel([&] { EXPECT_TRUE(ee->Cancel(handle)); });
    const utils::FastScopeGuard unblock([&]() noexcept {
        release.store(true);
        release.notify_all();
    });
    entered.WaitNonCancellable();
    auto shutdown = engine::AsyncNoTracing([&] { owner.Shutdown(); });
    while (!GetPosixEventEngine(owner)->IsStopping()) {
        engine::Yield();
    }
    EXPECT_FALSE(shutdown.IsFinished());
    release.store(true);
    release.notify_all();
    shutdown.Get();
    cancel.join();
}

}  // namespace
}  // namespace ugrpc::impl::event_engine

USERVER_NAMESPACE_END
