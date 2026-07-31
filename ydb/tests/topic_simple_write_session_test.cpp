#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vector>

#include <library/cpp/threading/future/future.h>

#include <userver/engine/deadline.hpp>
#include <userver/utest/utest.hpp>
#include <userver/ydb/tests/write_session_mock.hpp>
#include <userver/ydb/topic.hpp>

#include <ydb-cpp-sdk/client/types/status_codes.h>

USERVER_NAMESPACE_BEGIN

namespace {

using ::testing::_;
using ::testing::Return;

using WriteEvent = NYdb::NTopic::TWriteSessionEvent::TEvent;
using WriteEvents = std::vector<WriteEvent>;

class TopicSimpleWriteSessionTest : public ::testing::Test, public NYdb::NTopic::TContinuationTokenIssuer {
protected:
    static NThreading::TFuture<void> ReadyFuture() { return NThreading::MakeFuture(); }

    static NThreading::TFuture<std::uint64_t> InitSeqNoFuture(std::uint64_t seq_no) {
        return NThreading::MakeFuture<std::uint64_t>(seq_no);
    }

    WriteEvent MakeReadyEvent() {
        return NYdb::NTopic::TWriteSessionEvent::TReadyToAcceptEvent{IssueContinuationToken()};
    }

    static WriteEvent MakeAckEvent() {
        NYdb::NTopic::TWriteSessionEvent::TAcksEvent event;
        NYdb::NTopic::TWriteSessionEvent::TWriteAck ack;
        ack.SeqNo = 1;
        ack.State = NYdb::NTopic::TWriteSessionEvent::TWriteAck::EES_WRITTEN;
        event.Acks.push_back(std::move(ack));
        return event;
    }

    static WriteEvent MakeSessionClosedEvent() {
        return NYdb::NTopic::TSessionClosedEvent{NYdb::EStatus::SESSION_EXPIRED, NYdb::NIssue::TIssues{}};
    }

    const std::shared_ptr<ydb::tests::WriteSessionMock>& GetSession() const { return session_; }

private:
    std::shared_ptr<ydb::tests::WriteSessionMock> session_ = std::make_shared<ydb::tests::WriteSessionMock>();
};

UTEST_F(TopicSimpleWriteSessionTest, WriteConsumesReadyTokenAndWritesMessage) {
    ydb::TopicSimpleWriteSession writer{GetSession()};

    EXPECT_CALL(*GetSession(), WaitEvent()).WillOnce(Return(ReadyFuture()));
    EXPECT_CALL(*GetSession(), GetEvents(false, _)).WillOnce([&](bool, std::optional<std::size_t>) {
        WriteEvents events;
        events.push_back(MakeReadyEvent());
        return events;
    });
    EXPECT_CALL(*GetSession(), Write(_, _, nullptr)).Times(1);

    EXPECT_TRUE(writer.Write(NYdb::NTopic::TWriteMessage{"payload"}));
    EXPECT_TRUE(writer.IsAlive());
}

UTEST_F(TopicSimpleWriteSessionTest, WriteIgnoresAcksBeforeReadyToken) {
    ydb::TopicSimpleWriteSession writer{GetSession()};

    EXPECT_CALL(*GetSession(), WaitEvent()).Times(2).WillRepeatedly(Return(ReadyFuture()));
    EXPECT_CALL(*GetSession(), GetEvents(false, _))
        .WillOnce([](bool, std::optional<std::size_t>) {
            WriteEvents events;
            events.push_back(MakeAckEvent());
            return events;
        })
        .WillOnce([&](bool, std::optional<std::size_t>) {
            WriteEvents events;
            events.push_back(MakeReadyEvent());
            return events;
        });
    EXPECT_CALL(*GetSession(), Write(_, _, nullptr)).Times(1);

    EXPECT_TRUE(writer.Write(NYdb::NTopic::TWriteMessage{"payload"}));
    EXPECT_TRUE(writer.IsAlive());
}

UTEST_F(TopicSimpleWriteSessionTest, WriteReturnsFalseOnSessionClosedEvent) {
    ydb::TopicSimpleWriteSession writer{GetSession()};

    EXPECT_CALL(*GetSession(), WaitEvent()).WillOnce(Return(ReadyFuture()));
    EXPECT_CALL(*GetSession(), GetEvents(false, _)).WillOnce([](bool, std::optional<std::size_t>) {
        WriteEvents events;
        events.push_back(MakeSessionClosedEvent());
        return events;
    });
    EXPECT_CALL(*GetSession(), Write(_, _, nullptr)).Times(0);

    EXPECT_FALSE(writer.Write(NYdb::NTopic::TWriteMessage{"payload"}));
    EXPECT_FALSE(writer.IsAlive());
}

UTEST_F(TopicSimpleWriteSessionTest, WriteReturnsFalseOnPassedDeadlineWithoutToken) {
    ydb::TopicSimpleWriteSession writer{GetSession()};

    EXPECT_CALL(*GetSession(), WaitEvent()).WillOnce(Return(ReadyFuture()));
    EXPECT_CALL(*GetSession(), GetEvents(false, _)).WillOnce([](bool, std::optional<std::size_t>) {
        return WriteEvents{};
    });
    EXPECT_CALL(*GetSession(), Write(_, _, nullptr)).Times(0);

    EXPECT_FALSE(writer.Write(NYdb::NTopic::TWriteMessage{"payload"}, nullptr, engine::Deadline::Passed()));
    EXPECT_TRUE(writer.IsAlive());
}

UTEST_F(TopicSimpleWriteSessionTest, GetInitSeqNoUsesNativeFuture) {
    ydb::TopicSimpleWriteSession writer{GetSession()};

    EXPECT_CALL(*GetSession(), GetInitSeqNo()).WillOnce(Return(InitSeqNoFuture(42)));

    EXPECT_EQ(writer.GetInitSeqNo(), 42);
}

UTEST_F(TopicSimpleWriteSessionTest, CloseOnlyClosesNativeSessionOnce) {
    ydb::TopicSimpleWriteSession writer{GetSession()};

    EXPECT_CALL(*GetSession(), Close(_)).WillOnce(Return(true));

    EXPECT_TRUE(writer.Close(std::chrono::milliseconds{100}));
    EXPECT_TRUE(writer.Close(std::chrono::milliseconds{100}));
    EXPECT_FALSE(writer.IsAlive());
}

UTEST_F(TopicSimpleWriteSessionTest, GetNativeTopicWriteSession) {
    ydb::TopicSimpleWriteSession writer{GetSession()};

    EXPECT_EQ(&writer.GetNativeTopicWriteSession(), GetSession().get());
}

UTEST_F(TopicSimpleWriteSessionTest, MoveOperationsMarkSourceClosed) {
    ydb::TopicSimpleWriteSession writer{GetSession()};

    ydb::TopicSimpleWriteSession moved_writer{std::move(writer)};
    // NOLINTNEXTLINE(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
    EXPECT_FALSE(writer.IsAlive());
    EXPECT_TRUE(moved_writer.IsAlive());

    writer = std::move(moved_writer);
    // NOLINTNEXTLINE(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
    EXPECT_FALSE(moved_writer.IsAlive());
    EXPECT_TRUE(writer.IsAlive());
}

}  // namespace

USERVER_NAMESPACE_END
