#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>

#include <userver/utest/utest.hpp>
#include <userver/ydb/topic.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

using ::testing::_;
using ::testing::Return;

class ProducerMock final : public NYdb::NTopic::IProducer {
public:
    MOCK_METHOD(NYdb::NTopic::TWriteResult, Write, (NYdb::NTopic::TWriteMessage && message), (override));
    MOCK_METHOD(NThreading::TFuture<NYdb::NTopic::TFlushResult>, Flush, (), (override));
    MOCK_METHOD(NYdb::NTopic::TCloseResult, Close, (TDuration close_timeout), (override));
    MOCK_METHOD(NYdb::NTopic::TWriteStats, GetWriteStats, (), (override));
};

UTEST(TopicProducer, WriteForwardsQueued) {
    auto native_producer = std::make_shared<ProducerMock>();
    ydb::TopicProducer producer{native_producer};

    EXPECT_CALL(*native_producer, Write(_))
        .WillOnce(Return(NYdb::NTopic::TWriteResult{.Status = NYdb::NTopic::EWriteStatus::Queued, .ClosedDescription = {}}));

    EXPECT_TRUE(producer.Write(NYdb::NTopic::TWriteMessage{std::string{"payload"}}).IsQueued());
}

UTEST(TopicProducer, WriteForwardsOverloaded) {
    auto native_producer = std::make_shared<ProducerMock>();
    ydb::TopicProducer producer{native_producer};

    EXPECT_CALL(*native_producer, Write(_))
        .WillOnce(Return(NYdb::NTopic::TWriteResult{.Status = NYdb::NTopic::EWriteStatus::Timeout, .ClosedDescription = {}}));

    EXPECT_TRUE(producer.Write(NYdb::NTopic::TWriteMessage{std::string{"payload"}}).IsTimeout());
}

}  // namespace

USERVER_NAMESPACE_END
