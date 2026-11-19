#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

#include <userver/utest/utest.hpp>
#include <userver/utils/statistics/testing.hpp>
#include <userver/ydb/topic.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

using ::testing::_;
using ::testing::Return;
using ::testing::Throw;

class ProducerMock final : public NYdb::NTopic::IProducer {
public:
    MOCK_METHOD(NYdb::NTopic::TWriteResult, Write, (NYdb::NTopic::TWriteMessage && message), (override));
    MOCK_METHOD(NThreading::TFuture<NYdb::NTopic::TFlushResult>, Flush, (), (override));
    MOCK_METHOD(NYdb::NTopic::TCloseResult, Close, (TDuration close_timeout), (override));
    MOCK_METHOD(NYdb::NTopic::TWriteStats, GetWriteStats, (), (override));
};

NYdb::NTopic::TWriteResult MakeWriteResult(NYdb::NTopic::EWriteStatus status) {
    return NYdb::NTopic::TWriteResult{.Status = status, .ClosedDescription = {}};
}

UTEST(TopicProducer, WriteForwardsQueued) {
    auto native_producer = std::make_shared<ProducerMock>();
    ydb::TopicProducer producer{native_producer};

    EXPECT_CALL(*native_producer, Write(_)).WillOnce(Return(MakeWriteResult(NYdb::NTopic::EWriteStatus::Queued)));

    EXPECT_TRUE(producer.Write(NYdb::NTopic::TWriteMessage{std::string{"payload"}}).IsQueued());
}

UTEST(TopicProducer, WriteForwardsOverloaded) {
    auto native_producer = std::make_shared<ProducerMock>();
    ydb::TopicProducer producer{native_producer};

    EXPECT_CALL(*native_producer, Write(_)).WillOnce(Return(MakeWriteResult(NYdb::NTopic::EWriteStatus::Timeout)));

    EXPECT_TRUE(producer.Write(NYdb::NTopic::TWriteMessage{std::string{"payload"}}).IsTimeout());
}

UTEST(TopicProducer, CollectsWriteStatisticsAcrossCopies) {
    auto native_producer = std::make_shared<ProducerMock>();
    ydb::TopicProducer producer{native_producer};
    auto copy = producer;

    EXPECT_CALL(*native_producer, Write(_))
        .WillOnce(Return(MakeWriteResult(NYdb::NTopic::EWriteStatus::Queued)))
        .WillOnce(Return(MakeWriteResult(NYdb::NTopic::EWriteStatus::Timeout)))
        .WillOnce(Return(MakeWriteResult(NYdb::NTopic::EWriteStatus::Timeout)))
        .WillOnce(Return(MakeWriteResult(NYdb::NTopic::EWriteStatus::Error)))
        .WillOnce(Return(MakeWriteResult(NYdb::NTopic::EWriteStatus::Error)))
        .WillOnce(Throw(std::runtime_error{"write failed"}));
    EXPECT_CALL(*native_producer, GetWriteStats()).Times(0);

    EXPECT_TRUE(producer.Write(NYdb::NTopic::TWriteMessage{std::string{"queued"}}).IsQueued());
    EXPECT_TRUE(copy.Write(NYdb::NTopic::TWriteMessage{std::string{"timeout"}}).IsTimeout());
    EXPECT_TRUE(producer.Write(NYdb::NTopic::TWriteMessage{std::string{"second timeout"}}).IsTimeout());
    EXPECT_TRUE(producer.Write(NYdb::NTopic::TWriteMessage{std::string{"error"}}).IsError());
    EXPECT_TRUE(copy.Write(NYdb::NTopic::TWriteMessage{std::string{"second error"}}).IsError());
    EXPECT_THROW(producer.Write(NYdb::NTopic::TWriteMessage{std::string{"exception"}}), std::runtime_error);

    const utils::statistics::Snapshot snapshot{copy};
    EXPECT_EQ(snapshot.SingleMetric("write-attempts").AsRate(), 6);
    EXPECT_EQ(snapshot.SingleMetric("messages-queued").AsRate(), 1);
    EXPECT_EQ(snapshot.SingleMetric("buffer-timeouts").AsRate(), 2);
    EXPECT_EQ(snapshot.SingleMetric("write-errors").AsRate(), 3);
}

UTEST(TopicProducer, CountsOnlyCompletedWriteAttempts) {
    auto native_producer = std::make_shared<ProducerMock>();
    ydb::TopicProducer producer{native_producer};
    EXPECT_CALL(*native_producer, GetWriteStats()).Times(0);
    EXPECT_CALL(*native_producer, Write(_)).WillOnce([&producer](NYdb::NTopic::TWriteMessage&&) {
        const utils::statistics::Snapshot snapshot{producer};
        EXPECT_EQ(snapshot.SingleMetric("write-attempts").AsRate(), 0);
        return MakeWriteResult(NYdb::NTopic::EWriteStatus::Queued);
    });

    EXPECT_TRUE(producer.Write(NYdb::NTopic::TWriteMessage{std::string{"payload"}}).IsQueued());

    const utils::statistics::Snapshot snapshot{producer};
    EXPECT_EQ(snapshot.SingleMetric("write-attempts").AsRate(), 1);
}

UTEST(TopicProducer, DumpsWriteStatistics) {
    auto native_producer = std::make_shared<ProducerMock>();
    ydb::TopicProducer producer{native_producer};
    EXPECT_CALL(*native_producer, GetWriteStats()).Times(0);

    const utils::statistics::Snapshot snapshot{producer};

    EXPECT_EQ(snapshot.SingleMetric("write-attempts").AsRate(), 0);
    EXPECT_EQ(snapshot.SingleMetric("messages-queued").AsRate(), 0);
    EXPECT_EQ(snapshot.SingleMetric("buffer-timeouts").AsRate(), 0);
    EXPECT_EQ(snapshot.SingleMetric("write-errors").AsRate(), 0);
}

}  // namespace

USERVER_NAMESPACE_END
