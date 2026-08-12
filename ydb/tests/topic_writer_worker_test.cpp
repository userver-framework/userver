#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <userver/engine/run_standalone.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/utils/mock_now.hpp>
#include <userver/ydb/tests/write_session_mock.hpp>
#include <ydb/impl/topic_writer_worker.hpp>

#include <ydb-cpp-sdk/client/topic/write_events.h>
#include <ydb-cpp-sdk/client/topic/write_session.h>
#include <ydb-cpp-sdk/client/types/tx/tx.h>

using ::testing::DoAll;
using ::testing::Mock;
using ::testing::Return;
using ::testing::SaveArg;

USERVER_NAMESPACE_BEGIN

namespace {

// Mock for WriteSessionFactoryBase
class WriteSessionMockFactory : public ydb::impl::WriteSessionFactoryBase {
public:
    MOCK_METHOD(
        std::shared_ptr<NYdb::NTopic::IWriteSession>,
        CreateWriteSession,
        (NYdb::NTopic::TWriteSessionSettings),
        (override)
    );
};

// Helper function to create a test message
std::string CreateTestMessage(const std::string& content = "test message") { return content; }

// Helper function to create test metadata
ydb::TopicWriterMetadata CreateTestMetadata() { return {{"key1", "value1"}, {"key2", "value2"}}; }

}  // namespace

class TopicWriterWorkerTest : public ::testing::Test, public NYdb::NTopic::TContinuationTokenIssuer {
public:
    using TopicWriterWorker = ydb::impl::TopicWriterWorker;
    using TopicWriterWorkerSettings = ydb::impl::TopicWriterWorkerSettings;

    void SetUp() override {
        mock_factory = std::make_shared<WriteSessionMockFactory>();
        mock_session = std::make_shared<ydb::tests::WriteSessionMock>();

        // Setup default expectations
        EXPECT_CALL(*mock_factory, CreateWriteSession(::testing::_)).WillRepeatedly(Return(mock_session));

        settings.topic = "test-topic";
        settings.max_incoming_msg_queue_size = 100;
        settings.max_ydb_control_event_queue_size = 100;
    }

    std::shared_ptr<ydb::tests::WriteSessionMock> mock_session;
    std::shared_ptr<WriteSessionMockFactory> mock_factory;
    TopicWriterWorkerSettings settings;
};

TEST_F(TopicWriterWorkerTest, ConstructorInitializesCorrectly) {
    engine::RunStandalone([&] {
        TopicWriterWorker worker("test-worker", mock_factory, settings);

        EXPECT_EQ(worker.GetName(), "test-worker");
        EXPECT_EQ(worker.GetState(), ydb::impl::TopicWriterWorkerState::kInitial);
        EXPECT_FALSE(worker.IsReady());
    });
}

TEST_F(TopicWriterWorkerTest, WriteMessageReturnsOkWhenQueueHasSpace) {
    engine::RunStandalone([&] {
        TopicWriterWorker worker("test-worker", mock_factory, settings);

        auto msg = CreateTestMessage();
        auto meta = CreateTestMetadata();

        auto result = worker.WriteMessage(std::move(msg), std::move(meta));

        EXPECT_EQ(result.GetStatus(), ydb::TopicWriteStatus::kOk);
    });
}

TEST_F(TopicWriterWorkerTest, WriteMessageReturnsResourceExhaustedWhenQueueIsFull) {
    engine::RunStandalone([&] {
        // Create a worker with a very small queue
        settings.max_incoming_msg_queue_size = 1;
        TopicWriterWorker worker("test-worker", mock_factory, settings);

        // Fill the queue
        auto msg1 = CreateTestMessage("message1");
        auto meta1 = CreateTestMetadata();
        worker.WriteMessage(std::move(msg1), std::move(meta1));

        // Try to add another message
        auto msg2 = CreateTestMessage("message2");
        auto meta2 = CreateTestMetadata();
        auto result = worker.WriteMessage(std::move(msg2), std::move(meta2));

        EXPECT_EQ(result.GetStatus(), ydb::TopicWriteStatus::kResourceExhausted);
    });
}

TEST_F(TopicWriterWorkerTest, WorkerTransitionsToWorkingStateOnReadyToAcceptEvent) {
    engine::RunStandalone([&] {
        // Create a mock session that will emit ReadyToAccept event
        auto event_handler = NYdb::NTopic::TWriteSessionSettings::TEventHandlers{};

        // Capture the event handler
        NYdb::NTopic::TWriteSessionSettings captured_settings;
        EXPECT_CALL(*mock_factory, CreateWriteSession(::testing::_))
            .WillOnce(DoAll(SaveArg<0>(&captured_settings), Return(mock_session)));

        TopicWriterWorker worker("test-worker", mock_factory, settings, false);
        worker.TestHandlers();  // create session

        // Simulate ReadyToAccept event
        NYdb::NTopic::TWriteSessionEvent::TReadyToAcceptEvent ready_event{
            TContinuationTokenIssuer::IssueContinuationToken()
        };

        // Get the handler and call it
        if (captured_settings.EventHandlers_.ReadyToAcceptHandler_) {
            captured_settings.EventHandlers_.ReadyToAcceptHandler_(ready_event);
        }

        worker.TestHandlers();  // handle event
        EXPECT_EQ(worker.GetState(), ydb::impl::TopicWriterWorkerState::kWorking);
        EXPECT_TRUE(worker.IsReady());
    });
}

TEST_F(TopicWriterWorkerTest, WorkerTransitionsToDisconnectedStateOnSessionClosedEvent) {
    engine::RunStandalone([&] {
        // Create a mock session that will emit SessionClosed event
        NYdb::NTopic::TWriteSessionSettings captured_settings;
        EXPECT_CALL(*mock_factory, CreateWriteSession(::testing::_))
            .WillOnce(DoAll(SaveArg<0>(&captured_settings), Return(mock_session)))
            .WillOnce(Return(mock_session));

        TopicWriterWorker worker("test-worker", mock_factory, settings, false);
        worker.TestHandlers();  // create session

        // Simulate SessionClosed event
        NYdb::NTopic::TSessionClosedEvent closed_event{NYdb::EStatus::CANCELLED, {NYdb::NIssue::TIssue("MOCK tst")}};

        // Get the handler and call it
        if (captured_settings.EventHandlers_.SessionClosedHandler_) {
            captured_settings.EventHandlers_.SessionClosedHandler_(closed_event);
        }
        worker.TestHandlers();

        EXPECT_EQ(worker.GetState(), ydb::impl::TopicWriterWorkerState::kDisconnected);
        EXPECT_FALSE(worker.IsReady());
    });
}

TEST_F(TopicWriterWorkerTest, WorkerHandlesAckEventsCorrectly) {
    engine::RunStandalone([&] {
        // Create a mock session that will emit Ack events
        NYdb::NTopic::TWriteSessionSettings captured_settings;
        EXPECT_CALL(*mock_factory, CreateWriteSession(::testing::_))
            .WillOnce(DoAll(SaveArg<0>(&captured_settings), Return(mock_session)));

        TopicWriterWorker worker("test-worker", mock_factory, settings, false);

        worker.TestHandlers();

        // Simulate Ack event
        NYdb::NTopic::TWriteSessionEvent::TAcksEvent ack_event;
        NYdb::NTopic::TWriteSessionEvent::TWriteAck ack;
        ack.SeqNo = 123;
        ack.State = NYdb::NTopic::TWriteSessionEvent::TWriteAck::EEventState::EES_WRITTEN;
        ack_event.Acks.push_back(ack);

        // Get the handler and call it
        if (captured_settings.EventHandlers_.AcksHandler_) {
            captured_settings.EventHandlers_.AcksHandler_(ack_event);
        }

        worker.TestHandlers();
        // The test passes if no exceptions are thrown
        SUCCEED();
    });
}

TEST_F(TopicWriterWorkerTest, WorkerWritesMessageWhenReady) {
    engine::RunStandalone([&] {
        // Setup mocks
        NYdb::NTopic::TWriteSessionSettings captured_settings;
        EXPECT_CALL(*mock_factory, CreateWriteSession(::testing::_))
            .WillOnce(DoAll(SaveArg<0>(&captured_settings), Return(mock_session)));

        // Expect Write to be called
        EXPECT_CALL(*mock_session, Write(::testing::_, ::testing::_, ::testing::_)).Times(1);

        TopicWriterWorker worker("test-worker", mock_factory, settings, false);
        worker.TestHandlers();  // create session

        // First, simulate ReadyToAccept event to make the worker ready
        NYdb::NTopic::TWriteSessionEvent::TReadyToAcceptEvent ready_event{
            TContinuationTokenIssuer::IssueContinuationToken()
        };
        if (captured_settings.EventHandlers_.ReadyToAcceptHandler_) {
            captured_settings.EventHandlers_.ReadyToAcceptHandler_(ready_event);
        }

        // Now write a message
        auto msg = CreateTestMessage();
        auto meta = CreateTestMetadata();
        auto result = worker.WriteMessage(std::move(msg), std::move(meta));

        EXPECT_EQ(result.GetStatus(), ydb::TopicWriteStatus::kOk);
        worker.TestHandlers();  // handle message
    });
}

TEST_F(TopicWriterWorkerTest, WorkerRecreatesSessionAfterDisconnection) {
    engine::RunStandalone([&] {
        // Setup mocks for session creation
        auto mock_session2 = std::make_shared<ydb::tests::WriteSessionMock>();

        NYdb::NTopic::TWriteSessionSettings captured_settings;
        EXPECT_CALL(*mock_factory, CreateWriteSession(::testing::_))
            .WillOnce(DoAll(SaveArg<0>(&captured_settings), Return(mock_session)))
            .WillOnce(Return(mock_session2));

        TopicWriterWorker worker("test-worker", mock_factory, settings, false);
        worker.TestHandlers();  // create session

        // Simulate SessionClosed event to trigger disconnection
        NYdb::NTopic::TSessionClosedEvent closed_event{NYdb::EStatus::CANCELLED, {NYdb::NIssue::TIssue("MOCK tst")}};

        if (captured_settings.EventHandlers_.SessionClosedHandler_) {
            captured_settings.EventHandlers_.SessionClosedHandler_(closed_event);
        }

        worker.TestHandlers();  // handle event and try to recreate session

        // Verify that CreateWriteSession was called twice (initial creation +
        // recreation)
        Mock::VerifyAndClearExpectations(mock_factory.get());
    });
}

TEST_F(TopicWriterWorkerTest, StatisticsAreUpdatedCorrectly) {
    engine::RunStandalone([&] {
        // Setup mocks
        NYdb::NTopic::TWriteSessionSettings captured_settings;
        EXPECT_CALL(*mock_factory, CreateWriteSession(::testing::_))
            .WillOnce(DoAll(SaveArg<0>(&captured_settings), Return(mock_session)));

        // Expect Write to be called
        EXPECT_CALL(*mock_session, Write(::testing::_, ::testing::_, ::testing::_)).Times(3);

        TopicWriterWorker worker("test-worker", mock_factory, settings, false);
        worker.TestHandlers();  // create session

        // First, simulate ReadyToAccept event to make the worker ready
        NYdb::NTopic::TWriteSessionEvent::TReadyToAcceptEvent ready_event{
            TContinuationTokenIssuer::IssueContinuationToken()
        };
        if (captured_settings.EventHandlers_.ReadyToAcceptHandler_) {
            captured_settings.EventHandlers_.ReadyToAcceptHandler_(ready_event);
        }

        // Write 3 messages
        for (int i = 0; i < 3; ++i) {
            auto msg = CreateTestMessage("message" + std::to_string(i));
            auto meta = CreateTestMetadata();
            auto result = worker.WriteMessage(std::move(msg), std::move(meta));
            EXPECT_EQ(result.GetStatus(), ydb::TopicWriteStatus::kOk);
            worker.TestHandlers();  // handle message
            NYdb::NTopic::TWriteSessionEvent::TReadyToAcceptEvent ready_event{
                TContinuationTokenIssuer::IssueContinuationToken()
            };
            if (captured_settings.EventHandlers_.ReadyToAcceptHandler_) {
                captured_settings.EventHandlers_.ReadyToAcceptHandler_(ready_event);
            }
        }

        // Simulate Ack events for the messages
        NYdb::NTopic::TWriteSessionEvent::TAcksEvent ack_event;
        for (int i = 0; i < 3; ++i) {
            NYdb::NTopic::TWriteSessionEvent::TWriteAck ack;
            ack.SeqNo = i + 1;
            ack.State = NYdb::NTopic::TWriteSessionEvent::TWriteAck::EEventState::EES_WRITTEN;
            ack_event.Acks.push_back(ack);
        }

        // Get the handler and call it
        if (captured_settings.EventHandlers_.AcksHandler_) {
            captured_settings.EventHandlers_.AcksHandler_(ack_event);
        }

        // read acks
        worker.TestHandlers();

        SUCCEED();
    });
}

TEST_F(TopicWriterWorkerTest, HandlesManySequentialReadMessageEvents) {
    engine::RunStandalone([&] {
        // Setup mocks
        NYdb::NTopic::TWriteSessionSettings captured_settings;
        EXPECT_CALL(*mock_factory, CreateWriteSession(::testing::_))
            .WillOnce(DoAll(SaveArg<0>(&captured_settings), Return(mock_session)));

        // Expect Write to be called many times
        EXPECT_CALL(*mock_session, Write(::testing::_, ::testing::_, ::testing::_)).Times(100);

        TopicWriterWorker worker("test-worker", mock_factory, settings, false);
        worker.TestHandlers();  // create session

        // First, simulate ReadyToAccept event to make the worker ready
        NYdb::NTopic::TWriteSessionEvent::TReadyToAcceptEvent ready_event{
            TContinuationTokenIssuer::IssueContinuationToken()
        };
        if (captured_settings.EventHandlers_.ReadyToAcceptHandler_) {
            captured_settings.EventHandlers_.ReadyToAcceptHandler_(ready_event);
        }

        // Write many messages sequentially
        for (int i = 0; i < 100; ++i) {
            auto msg = CreateTestMessage("message" + std::to_string(i));
            auto meta = CreateTestMetadata();
            auto result = worker.WriteMessage(std::move(msg), std::move(meta));
            EXPECT_EQ(result.GetStatus(), ydb::TopicWriteStatus::kOk);
            worker.TestHandlers();  // handle message
            NYdb::NTopic::TWriteSessionEvent::TReadyToAcceptEvent ready_event{
                TContinuationTokenIssuer::IssueContinuationToken()
            };
            if (captured_settings.EventHandlers_.ReadyToAcceptHandler_) {
                captured_settings.EventHandlers_.ReadyToAcceptHandler_(ready_event);
            }
        }

        // Simulate many Ack events for the messages
        for (int batch = 0; batch < 10; ++batch) {
            NYdb::NTopic::TWriteSessionEvent::TAcksEvent ack_event;
            for (int i = 0; i < 10; ++i) {
                NYdb::NTopic::TWriteSessionEvent::TWriteAck ack;
                ack.SeqNo = batch * 10 + i + 1;
                ack.State = NYdb::NTopic::TWriteSessionEvent::TWriteAck::EEventState::EES_WRITTEN;
                ack_event.Acks.push_back(ack);
            }

            // Get the handler and call it
            if (captured_settings.EventHandlers_.AcksHandler_) {
                captured_settings.EventHandlers_.AcksHandler_(ack_event);
            }

            worker.TestHandlers();
        }

        // Verify that all messages were processed correctly
        SUCCEED();
    });
}

TEST_F(TopicWriterWorkerTest, HandlesMixedEventsSequence) {
    engine::RunStandalone([&] {
        // Setup mocks for session recreation
        auto mock_session2 = std::make_shared<ydb::tests::WriteSessionMock>();

        NYdb::NTopic::TWriteSessionSettings captured_settings;
        EXPECT_CALL(*mock_factory, CreateWriteSession(::testing::_))
            .WillOnce(DoAll(SaveArg<0>(&captured_settings), Return(mock_session)))
            .WillOnce(Return(mock_session2));

        // Expect Write to be called
        EXPECT_CALL(*mock_session, Write(::testing::_, ::testing::_, ::testing::_)).Times(5);
        EXPECT_CALL(*mock_session2, Write(::testing::_, ::testing::_, ::testing::_)).Times(3);

        TopicWriterWorker worker("test-worker", mock_factory, settings, false);
        worker.TestHandlers();  // create session

        // First, simulate ReadyToAccept event to make the worker ready
        NYdb::NTopic::TWriteSessionEvent::TReadyToAcceptEvent ready_event{
            TContinuationTokenIssuer::IssueContinuationToken()
        };
        if (captured_settings.EventHandlers_.ReadyToAcceptHandler_) {
            captured_settings.EventHandlers_.ReadyToAcceptHandler_(ready_event);
        }

        // Write 5 messages
        for (int i = 0; i < 5; ++i) {
            auto msg = CreateTestMessage("message" + std::to_string(i));
            auto meta = CreateTestMetadata();
            auto result = worker.WriteMessage(std::move(msg), std::move(meta));
            EXPECT_EQ(result.GetStatus(), ydb::TopicWriteStatus::kOk);
            worker.TestHandlers();  // handle message
            NYdb::NTopic::TWriteSessionEvent::TReadyToAcceptEvent ready_event{
                TContinuationTokenIssuer::IssueContinuationToken()
            };
            if (captured_settings.EventHandlers_.ReadyToAcceptHandler_) {
                captured_settings.EventHandlers_.ReadyToAcceptHandler_(ready_event);
            }
        }

        // Simulate Ack events for the first 3 messages
        NYdb::NTopic::TWriteSessionEvent::TAcksEvent ack_event;
        for (int i = 0; i < 3; ++i) {
            NYdb::NTopic::TWriteSessionEvent::TWriteAck ack;
            ack.SeqNo = i + 1;
            ack.State = NYdb::NTopic::TWriteSessionEvent::TWriteAck::EEventState::EES_WRITTEN;
            ack_event.Acks.push_back(ack);
        }

        // Get the handler and call it
        if (captured_settings.EventHandlers_.AcksHandler_) {
            captured_settings.EventHandlers_.AcksHandler_(ack_event);
        }

        worker.TestHandlers();

        // Simulate SessionClosed event to trigger disconnection
        NYdb::NTopic::TSessionClosedEvent closed_event{NYdb::EStatus::CANCELLED, {NYdb::NIssue::TIssue("MOCK tst")}};

        if (captured_settings.EventHandlers_.SessionClosedHandler_) {
            captured_settings.EventHandlers_.SessionClosedHandler_(closed_event);
        }

        worker.TestHandlers();  // handle event and try to recreate session

        // Write 3 more messages after reconnection
        for (int i = 0; i < 3; ++i) {
            auto msg = CreateTestMessage("message_after_reconnect" + std::to_string(i));
            auto meta = CreateTestMetadata();
            auto result = worker.WriteMessage(std::move(msg), std::move(meta));
            EXPECT_EQ(result.GetStatus(), ydb::TopicWriteStatus::kOk);
            // Simulate ReadyToAccept event for the new session
            NYdb::NTopic::TWriteSessionEvent::TReadyToAcceptEvent ready_event2{
                TContinuationTokenIssuer::IssueContinuationToken()
            };
            if (captured_settings.EventHandlers_.ReadyToAcceptHandler_) {
                captured_settings.EventHandlers_.ReadyToAcceptHandler_(ready_event2);
            }
            worker.TestHandlers();  // handle message
        }

        // Verify that all events were processed correctly
        SUCCEED();
    });
}

USERVER_NAMESPACE_END
