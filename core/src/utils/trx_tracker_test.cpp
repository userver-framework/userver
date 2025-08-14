#include <userver/utils/trx_tracker.hpp>

#include <gmock/gmock.h>

#include <userver/logging/log.hpp>
#include <userver/utest/log_capture_fixture.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/statistics/writer.hpp>

#include <utils/trx_tracker_internal.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

utils::statistics::Rate GetTriggers() { return utils::trx_tracker::GetStatistics().triggers; }

using TrxTrackerFixture = utest::LogCaptureFixture<>;

}  // namespace

UTEST_F(TrxTrackerFixture, AssertInTransaction) {
    utils::trx_tracker::ResetStatistics();
    const utils::trx_tracker::GlobalEnabler enabler;

    /// [Sample TransactionTracker usage]
    utils::trx_tracker::StartTransaction();
    utils::trx_tracker::CheckNoTransactions();
    utils::trx_tracker::EndTransaction();

    EXPECT_EQ(utils::trx_tracker::GetStatistics().triggers, 1);
    EXPECT_THAT(GetLogCapture().Filter("Long call while having active transactions"), testing::SizeIs(1));
    /// [Sample TransactionTracker usage]
}

UTEST(TrxTracker, AssertTwoInTransaction) {
    utils::trx_tracker::ResetStatistics();
    const utils::trx_tracker::GlobalEnabler enabler;

    utils::trx_tracker::StartTransaction();
    utils::trx_tracker::CheckNoTransactions();
    utils::trx_tracker::CheckNoTransactions();
    utils::trx_tracker::EndTransaction();

    EXPECT_EQ(GetTriggers(), 2);
}

UTEST(TrxTracker, AssertOutOfTransaction) {
    utils::trx_tracker::ResetStatistics();
    const utils::trx_tracker::GlobalEnabler enabler;

    utils::trx_tracker::StartTransaction();
    utils::trx_tracker::EndTransaction();
    utils::trx_tracker::CheckNoTransactions();

    EXPECT_EQ(GetTriggers(), 0);
}

UTEST(TrxTracker, AssertNestedTransactions) {
    utils::trx_tracker::ResetStatistics();
    const utils::trx_tracker::GlobalEnabler enabler;

    utils::trx_tracker::StartTransaction();
    utils::trx_tracker::StartTransaction();
    utils::trx_tracker::EndTransaction();
    utils::trx_tracker::CheckNoTransactions();
    utils::trx_tracker::EndTransaction();

    EXPECT_EQ(GetTriggers(), 1);
}

UTEST(TrxTracker, AssertWithDisabler) {
    utils::trx_tracker::ResetStatistics();
    const utils::trx_tracker::GlobalEnabler enabler;

    utils::trx_tracker::StartTransaction();
    const utils::trx_tracker::CheckDisabler disabler;
    utils::trx_tracker::CheckNoTransactions();
    utils::trx_tracker::EndTransaction();

    EXPECT_EQ(GetTriggers(), 0);
}

UTEST(TrxTracker, AssertDisablerReenabled) {
    utils::trx_tracker::ResetStatistics();
    const utils::trx_tracker::GlobalEnabler enabler;

    utils::trx_tracker::StartTransaction();
    utils::trx_tracker::CheckDisabler disabler;
    utils::trx_tracker::CheckNoTransactions();
    disabler.Reenable();
    utils::trx_tracker::CheckNoTransactions(utils::impl::SourceLocation::Current());
    utils::trx_tracker::EndTransaction();

    EXPECT_EQ(GetTriggers(), 1);
}

UTEST(TrxTracker, AssertDisablerDestroyed) {
    utils::trx_tracker::ResetStatistics();
    const utils::trx_tracker::GlobalEnabler enabler;

    utils::trx_tracker::StartTransaction();
    {
        const utils::trx_tracker::CheckDisabler disabler;
        utils::trx_tracker::CheckNoTransactions();
    }
    utils::trx_tracker::CheckNoTransactions();
    utils::trx_tracker::EndTransaction();

    EXPECT_EQ(GetTriggers(), 1);
}

UTEST(TrxTracker, AssertMultipleDisablers) {
    utils::trx_tracker::ResetStatistics();
    const utils::trx_tracker::GlobalEnabler enabler;

    utils::trx_tracker::StartTransaction();
    const utils::trx_tracker::CheckDisabler disabler;
    {
        const utils::trx_tracker::CheckDisabler disabler;
        utils::trx_tracker::CheckNoTransactions();
    }
    utils::trx_tracker::CheckNoTransactions();
    utils::trx_tracker::EndTransaction();

    EXPECT_EQ(GetTriggers(), 0);
}

UTEST(TrxTracker, NoGlobalEnabler) {
    utils::trx_tracker::ResetStatistics();

    utils::trx_tracker::StartTransaction();
    utils::trx_tracker::CheckNoTransactions();
    utils::trx_tracker::EndTransaction();

    EXPECT_EQ(utils::trx_tracker::GetStatistics().triggers, 0);
}

UTEST(TrxTracker, GlobalEnablerFalse) {
    utils::trx_tracker::ResetStatistics();
    const utils::trx_tracker::GlobalEnabler enabler(false);

    utils::trx_tracker::StartTransaction();
    utils::trx_tracker::CheckNoTransactions();
    utils::trx_tracker::EndTransaction();

    EXPECT_EQ(utils::trx_tracker::GetStatistics().triggers, 0);
}

USERVER_NAMESPACE_END
