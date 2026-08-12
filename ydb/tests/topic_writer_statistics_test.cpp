#include <gtest/gtest.h>
#include <cstddef>

#include <ydb/impl/topic_writer_statistics.hpp>

USERVER_NAMESPACE_BEGIN

class TopicWriterStatisticsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize test data
    }

    void TearDown() override {
        // Clean up
    }
};

TEST_F(TopicWriterStatisticsTest, WorkerEventStatsOperatorPlusEqualsWorksCorrectly) {
    // Create two TopicWriterEventStats objects with different values
    ydb::impl::TopicWriterEventStats stats1;
    ydb::impl::TopicWriterEventStats stats2;

    // Increment counters in stats1
    ++stats1.received;
    ++stats1.received;
    ++stats1.resource_exhasted;
    ++stats1.handled;
    ++stats1.handled_failed;

    // Increment counters in stats2
    ++stats2.received;
    ++stats2.resource_exhasted;
    ++stats2.resource_exhasted;
    ++stats2.handled;

    // Use operator+= to add stats2 to stats1
    stats1 += stats2;

    // Verify the results
    EXPECT_EQ(stats1.received.Load().value,
              static_cast<std::size_t>(3));  // 2 from stats1 + 1 from stats2
    EXPECT_EQ(stats1.resource_exhasted.Load().value,
              static_cast<std::size_t>(3));  // 1 from stats1 + 2 from stats2
    EXPECT_EQ(stats1.handled.Load().value,
              static_cast<std::size_t>(2));  // 1 from stats1 + 1 from stats2
    EXPECT_EQ(stats1.handled_failed.Load().value,
              static_cast<std::size_t>(1));  // 1 from stats1 + 0 from stats2
}

TEST_F(TopicWriterStatisticsTest, WorkerMessagesStatsOperatorPlusEqualsWorksCorrectly) {
    // Create two TopicWriterMessagesStats objects with different values
    ydb::impl::TopicWriterMessagesStats stats1;
    ydb::impl::TopicWriterMessagesStats stats2;

    // Increment counters in stats1
    ++stats1.received;
    ++stats1.received;
    ++stats1.resource_exhasted;
    ++stats1.handled;
    ++stats1.handled_failed;

    // Increment counters in stats2
    ++stats2.received;
    ++stats2.resource_exhasted;
    ++stats2.resource_exhasted;
    ++stats2.handled;

    // Use operator+= to add stats2 to stats1
    stats1 += stats2;

    // The test passes if no exceptions are thrown during the operation
    SUCCEED();
}

TEST_F(TopicWriterStatisticsTest, WorkerEventAcksStatsOperatorPlusEqualsWorksCorrectly) {
    // Create two TopicWriterEventAcksStats objects with different values
    ydb::impl::TopicWriterEventAcksStats stats1;
    ydb::impl::TopicWriterEventAcksStats stats2;

    // Manually increment counters in stats1
    for (int i = 0; i < 5; ++i) {
        ++stats1.written;
    }
    for (int i = 0; i < 3; ++i) {
        ++stats1.already_written;
    }
    for (int i = 0; i < 2; ++i) {
        ++stats1.discarded;
    }
    for (int i = 0; i < 4; ++i) {
        ++stats1.written_in_tx;
    }
    for (int i = 0; i < 1; ++i) {
        ++stats1.unexpected;
    }
    for (int i = 0; i < 7; ++i) {
        ++stats1.last_acks;
    }

    // Manually increment counters in stats2
    for (int i = 0; i < 2; ++i) {
        ++stats2.written;
    }
    for (int i = 0; i < 4; ++i) {
        ++stats2.already_written;
    }
    for (int i = 0; i < 1; ++i) {
        ++stats2.discarded;
    }
    for (int i = 0; i < 3; ++i) {
        ++stats2.written_in_tx;
    }
    for (int i = 0; i < 2; ++i) {
        ++stats2.unexpected;
    }
    for (int i = 0; i < 5; ++i) {
        ++stats2.last_acks;
    }

    // Use operator+= to add stats2 to stats1
    stats1 += stats2;

    // Verify the results
    EXPECT_EQ(stats1.written.Load().value, static_cast<std::size_t>(7));  // 5 + 2
    EXPECT_EQ(stats1.already_written.Load().value,
              static_cast<std::size_t>(7));                                 // 3 + 4
    EXPECT_EQ(stats1.discarded.Load().value, static_cast<std::size_t>(3));  // 2 + 1
    EXPECT_EQ(stats1.written_in_tx.Load().value,
              static_cast<std::size_t>(7));                                  // 4 + 3
    EXPECT_EQ(stats1.unexpected.Load().value, static_cast<std::size_t>(3));  // 1 + 2
    EXPECT_EQ(stats1.last_acks.Load(), static_cast<std::size_t>(12));        // 7 + 5
}

TEST_F(TopicWriterStatisticsTest, WorkerStatsOperatorPlusEqualsWorksCorrectly) {
    // Create two TopicWriterWorkerStats objects with different values
    ydb::impl::TopicWriterWorkerStats stats1;
    ydb::impl::TopicWriterWorkerStats stats2;

    // Increment counters in stats1
    ++stats1.main_cycle_issue;
    ++stats1.main_cycle_issue;
    ++stats1.write_issues;
    ++stats1.on_close_issue;

    // Increment event stats in stats1
    ++stats1.GetAckEventStats().received;
    ++stats1.GetAckEventStats().handled;

    ++stats1.GetReadyToAcceptEventStats().resource_exhasted;

    ++stats1.GetSessionClosedEventStats().handled_failed;

    // Increment message stats in stats1
    ++stats1.messages_stats.received;
    ++stats1.messages_stats.received;
    ++stats1.messages_stats.handled;

    // Increment event acks stats in stats1
    for (int i = 0; i < 5; ++i) {
        ++stats1.event_acks_stats.written;
    }
    for (int i = 0; i < 2; ++i) {
        ++stats1.event_acks_stats.discarded;
    }

    // Increment counters in stats2
    ++stats2.main_cycle_issue;
    ++stats2.write_issues;
    ++stats2.write_issues;
    ++stats2.on_close_issue;
    ++stats2.on_close_issue;

    // Increment event stats in stats2
    ++stats2.GetAckEventStats().received;
    ++stats2.GetAckEventStats().received;
    ++stats2.GetAckEventStats().handled_failed;

    ++stats2.GetReadyToAcceptEventStats().resource_exhasted;
    ++stats2.GetReadyToAcceptEventStats().resource_exhasted;

    ++stats2.GetSessionClosedEventStats().handled;

    // Increment message stats in stats2
    ++stats2.messages_stats.received;
    ++stats2.messages_stats.handled;
    ++stats2.messages_stats.handled_failed;

    // Increment event acks stats in stats2
    for (int i = 0; i < 3; ++i) {
        ++stats2.event_acks_stats.written;
    }
    for (int i = 0; i < 4; ++i) {
        ++stats2.event_acks_stats.already_written;
    }
    for (int i = 0; i < 1; ++i) {
        ++stats2.event_acks_stats.discarded;
    }

    // Use operator+= to add stats2 to stats1
    stats1 += stats2;

    // Verify the results for main counters
    EXPECT_EQ(stats1.main_cycle_issue.Load().value, static_cast<std::size_t>(3));  // 2 + 1
    EXPECT_EQ(stats1.write_issues.Load().value, static_cast<std::size_t>(3));      // 1 + 2
    EXPECT_EQ(stats1.on_close_issue.Load().value, static_cast<std::size_t>(3));    // 1 + 2

    // Verify the results for event stats
    EXPECT_EQ(stats1.GetAckEventStats().received.Load().value,
              static_cast<std::size_t>(3));  // 1 + 2
    EXPECT_EQ(stats1.GetAckEventStats().handled.Load().value,
              static_cast<std::size_t>(1));  // 1 + 0
    EXPECT_EQ(stats1.GetAckEventStats().handled_failed.Load().value,
              static_cast<std::size_t>(1));  // 0 + 1

    EXPECT_EQ(
        stats1.GetReadyToAcceptEventStats().resource_exhasted.Load().value,
        static_cast<std::size_t>(3)
    );  // 1 + 2

    EXPECT_EQ(stats1.GetSessionClosedEventStats().handled.Load().value,
              static_cast<std::size_t>(1));  // 0 + 1
    EXPECT_EQ(stats1.GetSessionClosedEventStats().handled_failed.Load().value,
              static_cast<std::size_t>(1));  // 1 + 0

    // Verify the results for message stats
    EXPECT_EQ(stats1.messages_stats.received.Load().value,
              static_cast<std::size_t>(3));  // 2 + 1
    EXPECT_EQ(stats1.messages_stats.handled.Load().value,
              static_cast<std::size_t>(2));  // 1 + 1
    EXPECT_EQ(stats1.messages_stats.handled_failed.Load().value,
              static_cast<std::size_t>(1));  // 0 + 1

    // Verify the results for event acks stats
    EXPECT_EQ(stats1.event_acks_stats.written.Load().value,
              static_cast<std::size_t>(8));  // 5 + 3
    EXPECT_EQ(stats1.event_acks_stats.already_written.Load().value,
              static_cast<std::size_t>(4));  // 0 + 4
    EXPECT_EQ(stats1.event_acks_stats.discarded.Load().value,
              static_cast<std::size_t>(3));  // 2 + 1
}

TEST_F(TopicWriterStatisticsTest, WorkerStatsOperatorPlusEqualsReturnsReference) {
    // Test that operator+= returns a reference to the object
    ydb::impl::TopicWriterWorkerStats stats1;
    ydb::impl::TopicWriterWorkerStats stats2;

    // Increment some counters
    ++stats1.main_cycle_issue;
    ++stats2.write_issues;

    // Use operator+= and check that it returns a reference
    ydb::impl::TopicWriterWorkerStats& result = (stats1 += stats2);

    // Verify that the result is the same object as stats1
    EXPECT_EQ(&result, &stats1);

    // Verify that the operation was performed correctly
    EXPECT_EQ(stats1.main_cycle_issue.Load().value, static_cast<std::size_t>(1));
    EXPECT_EQ(stats1.write_issues.Load().value, static_cast<std::size_t>(1));
}

TEST_F(TopicWriterStatisticsTest, WorkerStatsOperatorPlusEqualsWithEmptyStats) {
    // Test adding empty stats to non-empty stats
    ydb::impl::TopicWriterWorkerStats stats1;
    ydb::impl::TopicWriterWorkerStats stats2;  // Empty stats

    // Increment counters in stats1
    ++stats1.main_cycle_issue;
    ++stats1.write_issues;
    ++stats1.GetAckEventStats().received;
    ++stats1.messages_stats.handled;
    for (int i = 0; i < 5; ++i) {
        ++stats1.event_acks_stats.written;
    }

    // Use operator+= to add empty stats2 to stats1
    stats1 += stats2;

    // Verify that stats1 remains unchanged
    EXPECT_EQ(stats1.main_cycle_issue.Load().value, static_cast<std::size_t>(1));
    EXPECT_EQ(stats1.write_issues.Load().value, static_cast<std::size_t>(1));
    EXPECT_EQ(stats1.GetAckEventStats().received.Load().value, static_cast<std::size_t>(1));
    EXPECT_EQ(stats1.messages_stats.handled.Load().value, static_cast<std::size_t>(1));
    EXPECT_EQ(stats1.event_acks_stats.written.Load().value, static_cast<std::size_t>(5));
}

TEST_F(TopicWriterStatisticsTest, WorkerStatsOperatorPlusEqualsEmptyToEmpty) {
    // Test adding empty stats to empty stats
    ydb::impl::TopicWriterWorkerStats stats1;
    ydb::impl::TopicWriterWorkerStats stats2;  // Both empty

    // Use operator+= to add empty stats2 to empty stats1
    stats1 += stats2;

    // Verify that all counters are still zero
    EXPECT_EQ(stats1.main_cycle_issue.Load().value, static_cast<std::size_t>(0));
    EXPECT_EQ(stats1.write_issues.Load().value, static_cast<std::size_t>(0));
    EXPECT_EQ(stats1.on_close_issue.Load().value, static_cast<std::size_t>(0));
    EXPECT_EQ(stats1.GetAckEventStats().received.Load().value, static_cast<std::size_t>(0));
    EXPECT_EQ(stats1.GetAckEventStats().handled.Load().value, static_cast<std::size_t>(0));
    EXPECT_EQ(stats1.GetAckEventStats().handled_failed.Load().value, static_cast<std::size_t>(0));
    EXPECT_EQ(stats1.GetReadyToAcceptEventStats().received.Load().value, static_cast<std::size_t>(0));
    EXPECT_EQ(stats1.GetReadyToAcceptEventStats().handled.Load().value, static_cast<std::size_t>(0));
    EXPECT_EQ(stats1.GetReadyToAcceptEventStats().handled_failed.Load().value, static_cast<std::size_t>(0));
    EXPECT_EQ(stats1.GetSessionClosedEventStats().received.Load().value, static_cast<std::size_t>(0));
    EXPECT_EQ(stats1.GetSessionClosedEventStats().handled.Load().value, static_cast<std::size_t>(0));
    EXPECT_EQ(stats1.GetSessionClosedEventStats().handled_failed.Load().value, static_cast<std::size_t>(0));
    EXPECT_EQ(stats1.messages_stats.received.Load().value, static_cast<std::size_t>(0));
    EXPECT_EQ(stats1.messages_stats.handled.Load().value, static_cast<std::size_t>(0));
    EXPECT_EQ(stats1.messages_stats.handled_failed.Load().value, static_cast<std::size_t>(0));
    EXPECT_EQ(stats1.event_acks_stats.written.Load().value, static_cast<std::size_t>(0));
    EXPECT_EQ(stats1.event_acks_stats.already_written.Load().value, static_cast<std::size_t>(0));
    EXPECT_EQ(stats1.event_acks_stats.discarded.Load().value, static_cast<std::size_t>(0));
    EXPECT_EQ(stats1.event_acks_stats.written_in_tx.Load().value, static_cast<std::size_t>(0));
    EXPECT_EQ(stats1.event_acks_stats.unexpected.Load().value, static_cast<std::size_t>(0));
    EXPECT_EQ(stats1.event_acks_stats.last_acks.Load(), static_cast<std::size_t>(0));
}

USERVER_NAMESPACE_END
