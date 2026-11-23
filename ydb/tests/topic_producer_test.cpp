#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/current_task.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/statistics/testing.hpp>
#include <userver/ydb/topic.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

using ::testing::_;
using ::testing::Return;
using ::testing::StrictMock;
using ::testing::Throw;

static_assert(!std::is_copy_constructible_v<ydb::TopicProducer>);
static_assert(!std::is_copy_assignable_v<ydb::TopicProducer>);
static_assert(std::is_nothrow_move_constructible_v<ydb::TopicProducer>);
static_assert(std::is_nothrow_move_assignable_v<ydb::TopicProducer>);

class ProducerMock : public NYdb::NTopic::IProducer {
public:
    MOCK_METHOD(NYdb::NTopic::TWriteResult, Write, (NYdb::NTopic::TWriteMessage && message), (override));
    MOCK_METHOD(NThreading::TFuture<NYdb::NTopic::TFlushResult>, Flush, (), (override));
    MOCK_METHOD(NYdb::NTopic::TCloseResult, Close, (TDuration close_timeout), (override));
    MOCK_METHOD(NYdb::NTopic::TWriteStats, GetWriteStats, (), (override));
};

NYdb::NTopic::TWriteResult MakeWriteResult(NYdb::NTopic::EWriteStatus status) {
    return NYdb::NTopic::TWriteResult{.Status = status, .ClosedDescription = {}};
}

NYdb::NTopic::TCloseResult MakeCloseResult(NYdb::NTopic::ECloseStatus status) {
    return NYdb::NTopic::TCloseResult{.Status = status, .ClosedDescription = {}};
}

UTEST(TopicProducer, WriteForwardsQueued) {
    auto native_producer = std::make_shared<ProducerMock>();
    ydb::TopicProducer producer{utils::impl::InternalTag{}, native_producer, std::chrono::milliseconds::zero()};

    EXPECT_CALL(*native_producer, Write(_)).WillOnce(Return(MakeWriteResult(NYdb::NTopic::EWriteStatus::Queued)));

    EXPECT_TRUE(producer.Write(NYdb::NTopic::TWriteMessage{std::string{"payload"}}).IsQueued());
}

UTEST(TopicProducer, DestructorClosesWithConfiguredTimeoutOnBlockingTaskProcessor) {
    auto native_producer = std::make_shared<StrictMock<ProducerMock>>();
    auto& blocking_task_processor = engine::current_task::GetBlockingTaskProcessor();
    const auto close_timeout = std::chrono::milliseconds{123};
    EXPECT_CALL(*native_producer, Close(TDuration::MilliSeconds(close_timeout.count())))
        .WillOnce([&blocking_task_processor](TDuration) {
            EXPECT_EQ(&engine::current_task::GetTaskProcessor(), &blocking_task_processor);
            return MakeCloseResult(NYdb::NTopic::ECloseStatus::Success);
        });

    ydb::TopicProducer producer{utils::impl::InternalTag{}, native_producer, close_timeout};
}

UTEST(TopicProducer, ExplicitCloseIsNotRepeated) {
    auto native_producer = std::make_shared<StrictMock<ProducerMock>>();
    const auto close_timeout = std::chrono::milliseconds{123};

    EXPECT_CALL(*native_producer, Close(TDuration::MilliSeconds(close_timeout.count())))
        .WillOnce(Return(MakeCloseResult(NYdb::NTopic::ECloseStatus::Success)));

    ydb::TopicProducer producer{utils::impl::InternalTag{}, native_producer, std::chrono::milliseconds::zero()};
    EXPECT_TRUE(producer.Close(close_timeout).IsSuccess());
    EXPECT_TRUE(producer.Close(close_timeout).IsAlreadyClosed());
}

UTEST(TopicProducer, MoveTransfersCloseOwnership) {
    auto native_producer = std::make_shared<StrictMock<ProducerMock>>();
    const auto close_timeout = std::chrono::milliseconds{123};
    std::optional<ydb::TopicProducer> moved_producer;

    EXPECT_CALL(*native_producer, Close(_)).Times(0);
    {
        ydb::TopicProducer producer{utils::impl::InternalTag{}, native_producer, close_timeout};
        moved_producer.emplace(std::move(producer));
    }
    ASSERT_TRUE(::testing::Mock::VerifyAndClearExpectations(native_producer.get()));

    EXPECT_CALL(*native_producer, Close(TDuration::MilliSeconds(close_timeout.count())))
        .WillOnce(Return(MakeCloseResult(NYdb::NTopic::ECloseStatus::Success)));
    moved_producer.reset();
}

UTEST(TopicProducer, ExplicitCloseRunsOnBlockingTaskProcessor) {
    auto native_producer = std::make_shared<StrictMock<ProducerMock>>();
    auto& blocking_task_processor = engine::current_task::GetBlockingTaskProcessor();
    const auto close_timeout = std::chrono::milliseconds{123};
    EXPECT_CALL(*native_producer, Close(TDuration::MilliSeconds(close_timeout.count())))
        .WillOnce([&blocking_task_processor](TDuration) {
            EXPECT_EQ(&engine::current_task::GetTaskProcessor(), &blocking_task_processor);
            return MakeCloseResult(NYdb::NTopic::ECloseStatus::Success);
        });

    ydb::TopicProducer producer{utils::impl::InternalTag{}, native_producer, close_timeout};
    EXPECT_TRUE(producer.Close(close_timeout).IsSuccess());
}

UTEST(TopicProducer, CloseCompletesDespiteCallerCancellation) {
    auto task = utils::Async("cancelled-topic-producer-close", [] {
        auto native_producer = std::make_shared<StrictMock<ProducerMock>>();
        const auto close_timeout = std::chrono::milliseconds{123};
        EXPECT_CALL(*native_producer, Close(TDuration::MilliSeconds(close_timeout.count())))
            .WillOnce(Return(MakeCloseResult(NYdb::NTopic::ECloseStatus::Success)));

        ydb::TopicProducer producer{utils::impl::InternalTag{}, native_producer, close_timeout};
        engine::current_task::RequestCancel();
        EXPECT_TRUE(producer.Close(close_timeout).IsSuccess());
    });
    EXPECT_NO_THROW(task.Get());
}

UTEST(TopicProducer, WriteForwardsOverloaded) {
    auto native_producer = std::make_shared<ProducerMock>();
    ydb::TopicProducer producer{utils::impl::InternalTag{}, native_producer, std::chrono::milliseconds::zero()};

    EXPECT_CALL(*native_producer, Write(_)).WillOnce(Return(MakeWriteResult(NYdb::NTopic::EWriteStatus::Timeout)));

    EXPECT_TRUE(producer.Write(NYdb::NTopic::TWriteMessage{std::string{"payload"}}).IsTimeout());
}

UTEST(TopicProducer, CollectsWriteStatisticsAcrossMoves) {
    auto native_producer = std::make_shared<ProducerMock>();
    ydb::TopicProducer producer{utils::impl::InternalTag{}, native_producer, std::chrono::milliseconds::zero()};

    EXPECT_CALL(*native_producer, Write(_))
        .WillOnce(Return(MakeWriteResult(NYdb::NTopic::EWriteStatus::Queued)))
        .WillOnce(Return(MakeWriteResult(NYdb::NTopic::EWriteStatus::Timeout)))
        .WillOnce(Return(MakeWriteResult(NYdb::NTopic::EWriteStatus::Timeout)))
        .WillOnce(Return(MakeWriteResult(NYdb::NTopic::EWriteStatus::Error)))
        .WillOnce(Return(MakeWriteResult(NYdb::NTopic::EWriteStatus::Error)))
        .WillOnce(Throw(std::runtime_error{"write failed"}));
    EXPECT_CALL(*native_producer, GetWriteStats()).Times(0);

    EXPECT_TRUE(producer.Write(NYdb::NTopic::TWriteMessage{std::string{"queued"}}).IsQueued());
    auto moved_producer = std::move(producer);
    EXPECT_TRUE(moved_producer.Write(NYdb::NTopic::TWriteMessage{std::string{"timeout"}}).IsTimeout());
    EXPECT_TRUE(moved_producer.Write(NYdb::NTopic::TWriteMessage{std::string{"second timeout"}}).IsTimeout());
    EXPECT_TRUE(moved_producer.Write(NYdb::NTopic::TWriteMessage{std::string{"error"}}).IsError());
    EXPECT_TRUE(moved_producer.Write(NYdb::NTopic::TWriteMessage{std::string{"second error"}}).IsError());
    EXPECT_THROW(moved_producer.Write(NYdb::NTopic::TWriteMessage{std::string{"exception"}}), std::runtime_error);

    const utils::statistics::Snapshot snapshot{moved_producer};
    EXPECT_EQ(snapshot.SingleMetric("write-attempts").AsRate(), 6);
    EXPECT_EQ(snapshot.SingleMetric("messages-queued").AsRate(), 1);
    EXPECT_EQ(snapshot.SingleMetric("buffer-timeouts").AsRate(), 2);
    EXPECT_EQ(snapshot.SingleMetric("write-errors").AsRate(), 3);
}

UTEST(TopicProducer, CountsOnlyCompletedWriteAttempts) {
    auto native_producer = std::make_shared<ProducerMock>();
    ydb::TopicProducer producer{utils::impl::InternalTag{}, native_producer, std::chrono::milliseconds::zero()};
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
    ydb::TopicProducer producer{utils::impl::InternalTag{}, native_producer, std::chrono::milliseconds::zero()};
    EXPECT_CALL(*native_producer, GetWriteStats()).Times(0);

    const utils::statistics::Snapshot snapshot{producer};

    EXPECT_EQ(snapshot.SingleMetric("write-attempts").AsRate(), 0);
    EXPECT_EQ(snapshot.SingleMetric("messages-queued").AsRate(), 0);
    EXPECT_EQ(snapshot.SingleMetric("buffer-timeouts").AsRate(), 0);
    EXPECT_EQ(snapshot.SingleMetric("write-errors").AsRate(), 0);
}

}  // namespace

USERVER_NAMESPACE_END
