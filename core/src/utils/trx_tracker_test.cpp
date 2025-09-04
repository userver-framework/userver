#include <userver/utils/trx_tracker.hpp>

#include <gmock/gmock.h>

#include <userver/logging/log.hpp>
#include <userver/utest/log_capture_fixture.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

utils::statistics::Rate GetTriggers() { return utils::trx_tracker::GetStatistics().triggers; }

using TrxTrackerFixture = utest::LogCaptureFixture<>;

}  // namespace

UTEST_F(TrxTrackerFixture, AssertInTransaction) {
    utils::trx_tracker::ResetStatistics();
    const utils::trx_tracker::impl::GlobalEnabler enabler;

    /// [Sample TransactionTracker usage]
    utils::trx_tracker::TransactionLock trx{};
    trx.Lock();
    utils::trx_tracker::CheckNoTransactions();
    trx.Unlock();

    EXPECT_EQ(utils::trx_tracker::GetStatistics().triggers, 1);
    EXPECT_THAT(GetLogCapture().Filter("Long call while having active transactions"), testing::SizeIs(1));
    /// [Sample TransactionTracker usage]
}

UTEST(TrxTracker, AssertTwoInTransaction) {
    utils::trx_tracker::ResetStatistics();
    const utils::trx_tracker::impl::GlobalEnabler enabler;

    utils::trx_tracker::TransactionLock trx{};
    trx.Lock();
    utils::trx_tracker::CheckNoTransactions();
    utils::trx_tracker::CheckNoTransactions();
    trx.Unlock();

    EXPECT_EQ(GetTriggers(), 2);
}

UTEST(TrxTracker, AssertOutOfTransaction) {
    utils::trx_tracker::ResetStatistics();
    const utils::trx_tracker::impl::GlobalEnabler enabler;

    utils::trx_tracker::TransactionLock trx{};
    trx.Lock();
    trx.Unlock();
    utils::trx_tracker::CheckNoTransactions();

    EXPECT_EQ(GetTriggers(), 0);
}

UTEST(TrxTracker, UnlockOnDestruction) {
    utils::trx_tracker::ResetStatistics();
    const utils::trx_tracker::impl::GlobalEnabler enabler;

    {
        utils::trx_tracker::TransactionLock trx{};
        trx.Lock();
        utils::trx_tracker::CheckNoTransactions();
    }
    utils::trx_tracker::CheckNoTransactions();

    EXPECT_EQ(GetTriggers(), 1);
}

UTEST(TrxTracker, AssertNestedTransactions) {
    utils::trx_tracker::ResetStatistics();
    const utils::trx_tracker::impl::GlobalEnabler enabler;

    utils::trx_tracker::TransactionLock trx_1{};
    trx_1.Lock();
    utils::trx_tracker::TransactionLock trx_2{};
    trx_2.Lock();
    trx_2.Unlock();
    utils::trx_tracker::CheckNoTransactions();
    trx_1.Unlock();

    EXPECT_EQ(GetTriggers(), 1);
}

UTEST(TrxTracker, CopyConstructorLocked) {
    utils::trx_tracker::ResetStatistics();
    const utils::trx_tracker::impl::GlobalEnabler enabler;

    utils::trx_tracker::TransactionLock trx{};
    trx.Lock();
    utils::trx_tracker::TransactionLock trx_copy{std::move(trx)};
    utils::trx_tracker::CheckNoTransactions();
    trx_copy.Unlock();
    utils::trx_tracker::CheckNoTransactions();

    EXPECT_EQ(GetTriggers(), 1);
}

UTEST(TrxTracker, CopyConstructorUnlocked) {
    utils::trx_tracker::ResetStatistics();
    const utils::trx_tracker::impl::GlobalEnabler enabler;

    utils::trx_tracker::TransactionLock trx{};
    trx.Lock();
    utils::trx_tracker::CheckNoTransactions();
    trx.Unlock();
    utils::trx_tracker::TransactionLock trx_copy{std::move(trx)};
    utils::trx_tracker::CheckNoTransactions();

    EXPECT_EQ(GetTriggers(), 1);
}

UTEST(TrxTracker, CopyAssignmentLockedToLocked) {
    utils::trx_tracker::ResetStatistics();
    const utils::trx_tracker::impl::GlobalEnabler enabler;

    utils::trx_tracker::TransactionLock trx_1{};
    trx_1.Lock();
    utils::trx_tracker::TransactionLock trx_2{};
    trx_2.Lock();
    utils::trx_tracker::CheckNoTransactions();
    trx_2 = std::move(trx_1);
    utils::trx_tracker::CheckNoTransactions();
    trx_2.Unlock();
    utils::trx_tracker::CheckNoTransactions();

    EXPECT_EQ(GetTriggers(), 2);
}

UTEST(TrxTracker, CopyAssignmentLockedToUnlocked) {
    utils::trx_tracker::ResetStatistics();
    const utils::trx_tracker::impl::GlobalEnabler enabler;

    utils::trx_tracker::TransactionLock trx_1{};
    trx_1.Lock();
    utils::trx_tracker::TransactionLock trx_2{};
    utils::trx_tracker::CheckNoTransactions();
    trx_2 = std::move(trx_1);
    utils::trx_tracker::CheckNoTransactions();
    trx_2.Unlock();
    utils::trx_tracker::CheckNoTransactions();

    EXPECT_EQ(GetTriggers(), 2);
}

UTEST(TrxTracker, CopyAssignmentUnlockedToLocked) {
    utils::trx_tracker::ResetStatistics();
    const utils::trx_tracker::impl::GlobalEnabler enabler;

    utils::trx_tracker::TransactionLock trx_1{};
    utils::trx_tracker::TransactionLock trx_2{};
    trx_2.Lock();
    utils::trx_tracker::CheckNoTransactions();
    trx_2 = std::move(trx_1);
    utils::trx_tracker::CheckNoTransactions();

    EXPECT_EQ(GetTriggers(), 1);
}

UTEST(TrxTracker, CopyAssignmentUnlockedToUnlocked) {
    utils::trx_tracker::ResetStatistics();
    const utils::trx_tracker::impl::GlobalEnabler enabler;

    utils::trx_tracker::TransactionLock trx_1{};
    utils::trx_tracker::TransactionLock trx_2{};
    trx_2 = std::move(trx_1);
    utils::trx_tracker::CheckNoTransactions();

    EXPECT_EQ(GetTriggers(), 0);
}

UTEST(TrxTracker, CopyAssignmentSelf) {
    utils::trx_tracker::ResetStatistics();
    const utils::trx_tracker::impl::GlobalEnabler enabler;

    utils::trx_tracker::TransactionLock trx{};
    trx.Lock();
    auto& trx_ref = trx;
    trx = std::move(trx_ref);
    utils::trx_tracker::CheckNoTransactions();
    trx.Unlock();
    utils::trx_tracker::CheckNoTransactions();

    EXPECT_EQ(GetTriggers(), 1);
}

UTEST(TrxTracker, AssertWithDisabler) {
    utils::trx_tracker::ResetStatistics();
    const utils::trx_tracker::impl::GlobalEnabler enabler;

    utils::trx_tracker::TransactionLock trx{};
    trx.Lock();
    const utils::trx_tracker::CheckDisabler disabler;
    utils::trx_tracker::CheckNoTransactions();
    trx.Unlock();

    EXPECT_EQ(GetTriggers(), 0);
}

UTEST(TrxTracker, AssertDisablerReenabled) {
    utils::trx_tracker::ResetStatistics();
    const utils::trx_tracker::impl::GlobalEnabler enabler;

    utils::trx_tracker::TransactionLock trx{};
    trx.Lock();
    utils::trx_tracker::CheckDisabler disabler;
    utils::trx_tracker::CheckNoTransactions();
    disabler.Reenable();
    utils::trx_tracker::CheckNoTransactions(utils::impl::SourceLocation::Current());
    trx.Unlock();

    EXPECT_EQ(GetTriggers(), 1);
}

UTEST(TrxTracker, AssertDisablerDestroyed) {
    utils::trx_tracker::ResetStatistics();
    const utils::trx_tracker::impl::GlobalEnabler enabler;

    utils::trx_tracker::TransactionLock trx{};
    trx.Lock();
    {
        const utils::trx_tracker::CheckDisabler disabler;
        utils::trx_tracker::CheckNoTransactions();
    }
    utils::trx_tracker::CheckNoTransactions();
    trx.Unlock();

    EXPECT_EQ(GetTriggers(), 1);
}

UTEST(TrxTracker, AssertMultipleDisablers) {
    utils::trx_tracker::ResetStatistics();
    const utils::trx_tracker::impl::GlobalEnabler enabler;

    utils::trx_tracker::TransactionLock trx{};
    trx.Lock();
    const utils::trx_tracker::CheckDisabler disabler;
    {
        const utils::trx_tracker::CheckDisabler disabler;
        utils::trx_tracker::CheckNoTransactions();
    }
    utils::trx_tracker::CheckNoTransactions();
    trx.Unlock();

    EXPECT_EQ(GetTriggers(), 0);
}

UTEST(TrxTracker, NoGlobalEnabler) {
    utils::trx_tracker::ResetStatistics();

    utils::trx_tracker::TransactionLock trx{};
    trx.Lock();
    utils::trx_tracker::CheckNoTransactions();
    trx.Unlock();

    EXPECT_EQ(utils::trx_tracker::GetStatistics().triggers, 0);
}

UTEST(TrxTracker, GlobalEnablerFalse) {
    utils::trx_tracker::ResetStatistics();
    const utils::trx_tracker::impl::GlobalEnabler enabler(false);

    utils::trx_tracker::TransactionLock trx{};
    trx.Lock();
    utils::trx_tracker::CheckNoTransactions();
    trx.Unlock();

    EXPECT_EQ(utils::trx_tracker::GetStatistics().triggers, 0);
}

UTEST(TrxTracker, UnlockInAsync) {
    utils::trx_tracker::ResetStatistics();
    const utils::trx_tracker::impl::GlobalEnabler enabler;

    utils::trx_tracker::TransactionLock trx{};
    trx.Lock();
    auto task = utils::Async("", [&trx]() { trx.Unlock(); });
    task.Get();
    utils::trx_tracker::CheckNoTransactions();
    trx.Unlock();
    utils::trx_tracker::CheckNoTransactions();

    EXPECT_EQ(GetTriggers(), 1);
}

UTEST(TrxTracker, LockInAsync) {
    utils::trx_tracker::ResetStatistics();
    const utils::trx_tracker::impl::GlobalEnabler enabler;

    auto task = utils::Async("", []() {
        utils::trx_tracker::TransactionLock trx{};
        trx.Lock();
        return trx;
    });
    auto trx = task.Get();
    utils::trx_tracker::CheckNoTransactions();
    trx.Unlock();
    utils::trx_tracker::CheckNoTransactions();

    EXPECT_EQ(GetTriggers(), 0);
}

USERVER_NAMESPACE_END
