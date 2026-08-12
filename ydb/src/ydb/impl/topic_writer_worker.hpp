#pragma once

#include <atomic>
#include <optional>
#include <string>
#include <variant>

#include <userver/concurrent/mpsc_queue.hpp>
#include <userver/concurrent/queue.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/datetime.hpp>
#include <userver/utils/statistics/writer.hpp>
#include <userver/ydb/topic.hpp>

#include <ydb-cpp-sdk/client/topic/client.h>
#include <ydb-cpp-sdk/client/topic/write_session.h>

#include <userver/ydb/topic_writer_types.hpp>

#include <ydb/impl/topic_writer_impl_types.hpp>
#include <ydb/impl/topic_writer_statistics.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb::impl {

/**
   XXXXXXXXX    | kInitial | kWorking | kDisconnected
   --------------------------------------------------
   kInitial     |    NA    |    YES   |     YES
   --------------------------------------------------
   kWorking     |    NO    |    NA    |     YES
   --------------------------------------------------
   kDisconnected|    NO    |    YES   |     NA

  kInitial - session object has been created, but there no any messages at the
  moment
 */

using TopicWriterMessage = ydb::impl::TopicWriterMessage;

struct TopicWriterMessageItem {
    TopicWriterMessage message;
    TopicWriterMetadata metadata;
};

struct TopicWriterSessionData {
    /**
       backoff is used for Ydb session recreation
       if we unable to create session right now, the next attempt
       has to be delayed
     */
    struct BackoffSettings {
        std::chrono::milliseconds initial{500};
        std::chrono::milliseconds max{30000};
        double multiplier{2};
    };

    void CleanSessionData(bool recreate) {
        if (recreate) {
            // first issues with session
            current_recreate_count = 0;
        } else {
            current_recreate_count++;
        }
        session = nullptr;
        token = std::nullopt;
    }

    void StepRecreateCount() { current_recreate_count++; }

    engine::Deadline GetDelayForError() const;

    std::shared_ptr<NYdb::NTopic::IWriteSession> session;
    std::optional<NYdb::NTopic::TContinuationToken> token{};

    BackoffSettings backoff;

    std::size_t current_recreate_count{0};
};

using TopicWriterYdbEventItem = std::variant<
    NYdb::NTopic::TWriteSessionEvent::TAcksEvent,
    NYdb::NTopic::TWriteSessionEvent::TReadyToAcceptEvent,
    NYdb::NTopic::TSessionClosedEvent>;

class WriteSessionFactoryBase {
public:
    virtual std::shared_ptr<NYdb::NTopic::IWriteSession> CreateWriteSession(NYdb::NTopic::TWriteSessionSettings) = 0;
    virtual ~WriteSessionFactoryBase() = default;
};

class WriteSessionFactory : public WriteSessionFactoryBase {
public:
    WriteSessionFactory(std::shared_ptr<ydb::TopicClient> ydb_topic_client)
        : ydb_topic_client_{std::move(ydb_topic_client)}
    {}

    std::shared_ptr<NYdb::NTopic::IWriteSession> CreateWriteSession(NYdb::NTopic::TWriteSessionSettings) override;

    ~WriteSessionFactory() override = default;

private:
    std::shared_ptr<ydb::TopicClient> ydb_topic_client_;
};

class TopicWriterWorkerBase {
public:
    virtual TopicWriteResult WriteMessage(std::string msg, TopicWriterMetadata meta) = 0;
    virtual ~TopicWriterWorkerBase() = default;
};

struct TopicWriterWorkerSettings {
    /// topic to be handled
    std::string topic;

    /// size for the queue with incoming message
    std::size_t max_incoming_msg_queue_size{};

    /// size for the queue with ydb control messages
    std::size_t max_ydb_control_event_queue_size{};
};

/**
   @brief Implements main logic for work with YDB
   Object includes three queues:
   - MPSC - to hold incoming messages, before writing to YDB
            MPSC is selected in order to allow writing from different coroutines
   - SPSC for critical events: TReadyToAcceptEvent and TSessionClosedEvent
          we use SPSC because YDB does not use coroutines but threads, and we
   have to take event for handling from separate thread
   - SPSC for acks event: we use separate queue for acks event in order to
   exclude situation when the queue with event is filled and there is no place
   to handle critical events like TReadyToAcceptEvent and TSessionClosedEvent

   The object in loop check the following things:
   - if we have control events (TReadyToAcceptEvent, TSessionClosedEvent)
   - if we have messages to be handled, if yes than the message is sent to the
   ydb
   - if we need to recreate session
 */
class TopicWriterWorker final : public TopicWriterWorkerBase {
public:
    TopicWriterWorker(std::string name, std::shared_ptr<WriteSessionFactoryBase> factory, TopicWriterWorkerSettings);
    // For tests purpose
    TopicWriterWorker(
        std::string name,
        std::shared_ptr<WriteSessionFactoryBase> factory,
        TopicWriterWorkerSettings,
        bool run_loop
    );

    TopicWriteResult WriteMessage(std::string message, TopicWriterMetadata meta) override;

    TopicWriterWorkerState GetState() const { return state_.load(); }

    bool IsReady() const { return GetState() == TopicWriterWorkerState::kWorking; }

    const std::string& GetName() const { return name_; }

    // For test purpose only
    const TopicWriterWorkerStats& GetStatistics() const { return stats_; }

    ~TopicWriterWorker() override;

    // For test purpose only
    void TestHandlers();

private:
    using Queue = concurrent::MpscQueue<TopicWriterMessageItem>;

    using YdbEventQueue = concurrent::SpscQueue<TopicWriterYdbEventItem>;
    using YdbEventQueueProducer = YdbEventQueue::Producer;

    void RunWriteTask();

    std::shared_ptr<NYdb::NTopic::IWriteSession> CreateWriteSession();
    void CloseSessionGracefully();
    void WaitCloseSessionEvent();

    NYdb::NTopic::TWriteSessionSettings CreateWriteSessionSettings();

    void ChangeState(TopicWriterWorkerState new_state);

    void WriteTask();

    // handles only TReadyToAcceptEvent and TSessionClosedEvent
    void HandleCriticalEvent();
    void HandleAckEvent();

    void HandleYdbEvent();
    void HandleMessage();

    void HandleYdbEvent(TopicWriterYdbEventItem& event);
    void HandleYdbEvent(NYdb::NTopic::TWriteSessionEvent::TAcksEvent& event);
    void HandleYdbEvent(NYdb::NTopic::TWriteSessionEvent::TReadyToAcceptEvent& event);
    void HandleYdbEvent(NYdb::NTopic::TSessionClosedEvent& event);

    void CleanWriteSession();
    void ReInitWriteSession();

    friend void DumpMetric(utils::statistics::Writer& writer, const TopicWriterWorker& worker);

    const std::string name_;
    std::shared_ptr<WriteSessionFactoryBase> factory_;

    TopicWriterWorkerSettings settings_;

    TopicWriterWorkerStats stats_;

    std::atomic<TopicWriterWorkerState> state_{TopicWriterWorkerState::kInitial};

    std::shared_ptr<Queue> queue_;
    Queue::MultiProducer producer_;
    Queue::Consumer consumer_;

    std::shared_ptr<YdbEventQueue> event_queue_;
    YdbEventQueue::Producer event_producer_;
    YdbEventQueue::Consumer event_consumer_;

    // We can receive a lot of Acks messages and it is possible situation
    // when the internal queue for events is overflowed.
    // It could be critical for Continuation token receiving
    // so, let's have separate queue for acks
    std::shared_ptr<YdbEventQueue> ack_event_queue_;
    YdbEventQueue::Producer ack_event_producer_;
    YdbEventQueue::Consumer ack_event_consumer_;

    TopicWriterSessionData write_session_;

    // Must be the last field: the task uses other fields above and must be
    // destroyed first to ensure correct lifetime ordering.
    engine::Task write_task_;
};

void DumpMetric(utils::statistics::Writer& writer, const TopicWriterWorker& worker);

}  // namespace ydb::impl

USERVER_NAMESPACE_END
