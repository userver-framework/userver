#include <userver/ugrpc/impl/async_method_invocation.hpp>

#include <atomic>
#include <chrono>
#include <optional>
#include <thread>
#include <type_traits>

#include <userver/engine/async.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/single_use_event.hpp>
#include <userver/engine/task/current_task.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/fast_scope_guard.hpp>

#include <userver/ugrpc/impl/event_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

using ugrpc::impl::AsyncMethodInvocation;
using WaitStatus = AsyncMethodInvocation::WaitStatus;

static_assert(!std::is_polymorphic_v<AsyncMethodInvocation>);

constexpr int kConcurrentDestructionIterations = 100;

template <typename CancelWait>
class GrpcAsyncMethodInvocationDestruction : public ::testing::Test {};

using CancellationModes = ::testing::Types<std::false_type, std::true_type>;

}  // namespace

TEST(GrpcAsyncMethodInvocation, UnusedInvocationCanBeDestroyedOutsideCoroutine) {
    const AsyncMethodInvocation invocation;
}

UTEST(GrpcAsyncMethodInvocation, CompletionReportsSuccess) {
    AsyncMethodInvocation invocation;
    auto* const tag = static_cast<ugrpc::impl::EventBase*>(invocation.GetCompletionTag());
    EXPECT_FALSE(invocation.IsReady());
    tag->Notify(true);
    EXPECT_TRUE(invocation.IsReady());
    EXPECT_EQ(invocation.Wait(), WaitStatus::kOk);
    EXPECT_TRUE(invocation.WaitNonCancellable());
}

UTEST(GrpcAsyncMethodInvocation, CompletionReportsFailure) {
    AsyncMethodInvocation invocation;
    auto* const tag = static_cast<ugrpc::impl::EventBase*>(invocation.GetCompletionTag());
    tag->Notify(false);
    EXPECT_TRUE(invocation.IsReady());
    EXPECT_EQ(invocation.Wait(), WaitStatus::kError);
    EXPECT_FALSE(invocation.WaitNonCancellable());
}

TYPED_UTEST_SUITE(GrpcAsyncMethodInvocationDestruction, CancellationModes);

TYPED_UTEST(GrpcAsyncMethodInvocationDestruction, DestructorWaitsForCompletion) {
    constexpr bool kCancelWait = TypeParam::value;
    constexpr auto kExpectedWaitStatus = kCancelWait ? WaitStatus::kCancelled : WaitStatus::kDeadline;

    std::optional<AsyncMethodInvocation> invocation{std::in_place};
    auto* const tag = static_cast<ugrpc::impl::EventBase*>(invocation->GetCompletionTag());
    engine::SingleUseEvent start_wait;
    engine::SingleUseEvent wait_finished;

    auto destruction = engine::CriticalAsyncNoTracing([&] {
        start_wait.WaitNonCancellable();
        EXPECT_EQ(
            invocation->WaitUntil(engine::Deadline::FromDuration(std::chrono::seconds::zero())),
            kExpectedWaitStatus
        );
        wait_finished.Send();
        invocation.reset();
    });

    if constexpr (kCancelWait) {
        destruction.RequestCancel();
    }
    start_wait.Send();
    wait_finished.WaitNonCancellable();
    ASSERT_FALSE(destruction.IsFinished());

    tag->Notify(true);
    destruction.Get();
    EXPECT_FALSE(invocation.has_value());
}

TYPED_UTEST(GrpcAsyncMethodInvocationDestruction, CompletionCanRaceWithDestruction) {
    constexpr bool kCancelWait = TypeParam::value;

    for (int iteration = 0; iteration < kConcurrentDestructionIterations; ++iteration) {
        std::optional<AsyncMethodInvocation> invocation{std::in_place};
        auto* const tag = static_cast<ugrpc::impl::EventBase*>(invocation->GetCompletionTag());
        std::atomic<bool> destruction_started{false};
        std::thread notifier([tag, &destruction_started] {
            destruction_started.wait(false, std::memory_order_relaxed);
            tag->Notify(true);
        });
        const utils::FastScopeGuard join_notifier([&]() noexcept {
            destruction_started.store(true, std::memory_order_relaxed);
            destruction_started.notify_one();
            engine::CriticalAsyncNoTracing(engine::current_task::GetBlockingTaskProcessor(), [&] {
                notifier.join();
            }).Get();
        });

        engine::SingleUseEvent start_destruction;
        auto destruction = engine::CriticalAsyncNoTracing([&] {
            start_destruction.WaitNonCancellable();
            if constexpr (kCancelWait) {
                EXPECT_EQ(invocation->Wait(), WaitStatus::kCancelled);
            }
            destruction_started.store(true, std::memory_order_relaxed);
            destruction_started.notify_one();
            invocation.reset();
        });
        if constexpr (kCancelWait) {
            destruction.RequestCancel();
        }
        start_destruction.Send();
        destruction.Get();
    }
}

USERVER_NAMESPACE_END
