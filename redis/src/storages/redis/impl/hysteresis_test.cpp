#include "hysteresis.hpp"

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

TEST(HysteresisTest, DefaultDoesNotChangeState) {
    // By default, no thresholds are set, so state never changes
    Hysteresis h;

    EXPECT_FALSE(h.IsFailed()) << "Default hysteresis should not be failed initially";

    for (size_t i = 0; i < 10; ++i) {
        h.AccountFail(true);
    }

    EXPECT_FALSE(h.IsFailed()) << "Default hysteresis should not become failed";
}

TEST(HysteresisTest, FailureThresholdTriggersFailedState) {
    Hysteresis::Config cfg{.consecutive_failures = 2};
    Hysteresis h(cfg);

    // First failure – not enough to trigger
    h.AccountFail(true);
    EXPECT_FALSE(h.IsFailed()) << "Failed after 1 failure, need 2";

    // Second consecutive failure – should trigger failed state
    h.AccountFail(true);
    EXPECT_TRUE(h.IsFailed()) << "Should be failed after reaching failure threshold";

    // Success should not clear the failed state because consecutive_ok is 0
    h.AccountFail(false);
    EXPECT_TRUE(h.IsFailed()) << "Failed state should persist when consecutive_ok is 0";
}

TEST(HysteresisTest, SuccessThresholdClearsFailedState) {
    Hysteresis::Config cfg{.consecutive_failures = 2, .consecutive_ok = 3};
    Hysteresis h(cfg);

    // Trigger failed state
    h.AccountFail(true);
    h.AccountFail(true);
    EXPECT_TRUE(h.IsFailed()) << "Failed state should be set after 2 failures";

    // One success – not enough to clear
    h.AccountFail(false);
    EXPECT_TRUE(h.IsFailed()) << "Failed state should remain after 1 success";

    // Second success – still not enough
    h.AccountFail(false);
    EXPECT_TRUE(h.IsFailed()) << "Failed state should remain after 2 successes";

    // Third consecutive success – should clear failed state
    h.AccountFail(false);
    EXPECT_FALSE(h.IsFailed()) << "Failed state should be cleared after reaching success threshold";
}

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
