#include "command_admission.hpp"

#include <atomic>
#include <optional>
#include <thread>
#include <type_traits>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

static_assert(std::is_move_assignable_v<CommandAdmission::Permit>);

TEST(CommandAdmission, PermitLifetimeAndClose) {
    CommandAdmission admission;
    auto permit = admission.TryAcquire();
    ASSERT_TRUE(permit);

    std::optional<CommandAdmission::Permit> moved_permit{std::move(*permit)};
    permit.reset();

    EXPECT_TRUE(admission.Close());
    EXPECT_FALSE(admission.Close());
    EXPECT_FALSE(admission.TryAcquire());

    std::atomic<bool> wait_started{false};
    std::atomic<bool> wait_finished{false};
    std::thread waiter([&] {
        wait_started.store(true, std::memory_order_release);
        wait_started.notify_one();
        admission.WaitForNoActivePermits();
        wait_finished.store(true, std::memory_order_release);
    });

    wait_started.wait(false, std::memory_order_acquire);
    EXPECT_FALSE(wait_finished.load(std::memory_order_acquire));

    moved_permit.reset();
    waiter.join();
    EXPECT_TRUE(wait_finished.load(std::memory_order_acquire));
}

TEST(CommandAdmission, LastPermitReleasedBeforeWait) {
    CommandAdmission admission;
    auto permit = admission.TryAcquire();
    ASSERT_TRUE(permit);

    EXPECT_TRUE(admission.Close());
    permit.reset();

    admission.WaitForNoActivePermits();
}

TEST(CommandAdmission, PermitMoveAssignmentReleasesPreviousPermit) {
    CommandAdmission admission;
    auto first = admission.TryAcquire();
    auto second = admission.TryAcquire();
    ASSERT_TRUE(first);
    ASSERT_TRUE(second);

    *first = std::move(*second);
    second.reset();
    ASSERT_TRUE(admission.Close());

    std::atomic<bool> wait_started{false};
    std::atomic<bool> wait_finished{false};
    std::thread waiter([&] {
        wait_started.store(true, std::memory_order_release);
        wait_started.notify_one();
        admission.WaitForNoActivePermits();
        wait_finished.store(true, std::memory_order_release);
    });

    wait_started.wait(false, std::memory_order_acquire);
    EXPECT_FALSE(wait_finished.load(std::memory_order_acquire));

    first.reset();
    waiter.join();
    EXPECT_TRUE(wait_finished.load(std::memory_order_acquire));
}

TEST(CommandAdmission, WakesAllWaiters) {
    CommandAdmission admission;
    auto permit = admission.TryAcquire();
    ASSERT_TRUE(permit);
    ASSERT_TRUE(admission.Close());

    std::atomic<int> waiters_started{0};
    std::atomic<int> waiters_finished{0};
    const auto wait = [&] {
        waiters_started.fetch_add(1, std::memory_order_release);
        waiters_started.notify_all();
        admission.WaitForNoActivePermits();
        waiters_finished.fetch_add(1, std::memory_order_release);
    };
    std::thread first_waiter{wait};
    std::thread second_waiter{wait};

    auto started = waiters_started.load(std::memory_order_acquire);
    while (started != 2) {
        waiters_started.wait(started, std::memory_order_acquire);
        started = waiters_started.load(std::memory_order_acquire);
    }
    EXPECT_EQ(waiters_finished.load(std::memory_order_acquire), 0);

    permit.reset();
    first_waiter.join();
    second_waiter.join();
    EXPECT_EQ(waiters_finished.load(std::memory_order_acquire), 2);
}

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
