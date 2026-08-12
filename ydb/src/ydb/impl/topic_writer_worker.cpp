#include "topic_writer_worker.hpp"

#include <chrono>

#include <userver/engine/async.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/current_task.hpp>
#include <userver/logging/log.hpp>
#include <userver/testsuite/testpoint.hpp>
#include <userver/utils/overloaded.hpp>
#include <userver/utils/rand.hpp>
#include <userver/utils/statistics/relaxed_counter.hpp>

#include <util/datetime/base.h>

USERVER_NAMESPACE_BEGIN

namespace ydb::impl {

namespace {

constexpr std::chrono::milliseconds kDefaultCloseTmo{1000};
constexpr std::chrono::milliseconds kDefaultEventPopTmo{100};
constexpr std::chrono::milliseconds kWaitCloseSessionEventOnExit{1000};
constexpr std::chrono::milliseconds kDefaultMessagePopTmo{100};
constexpr std::size_t kMaxAckYdbEventQueueSize{100000};
constexpr std::size_t kMaxAckEventReadSliceSize{1000};

}  // namespace

std::shared_ptr<NYdb::NTopic::IWriteSession> WriteSessionFactory::CreateWriteSession(
    NYdb::NTopic::TWriteSessionSettings settings
) {
    return ydb_topic_client_->GetNativeTopicClient().CreateWriteSession(settings);
}

engine::Deadline TopicWriterSessionData::GetDelayForError() const {
    const auto current_tmo = std::chrono::ceil<std::chrono::milliseconds>(
        backoff.initial * std::ceil(std::pow(backoff.multiplier, static_cast<int>(current_recreate_count)))
    );

    const auto backoff_tmo = std::min(current_tmo, backoff.max);
    const auto backoff_tmo_with_jitter = utils::RandRange(backoff_tmo.count() / 2, backoff_tmo.count());
    return engine::Deadline::FromDuration(std::chrono::milliseconds{backoff_tmo_with_jitter});
}

TopicWriterWorker::TopicWriterWorker(
    std::string name,
    std::shared_ptr<WriteSessionFactoryBase> factory,
    TopicWriterWorkerSettings settings
)
    : TopicWriterWorker(std::move(name), std::move(factory), std::move(settings), true)
{}

TopicWriterWorker::TopicWriterWorker(
    std::string name,
    std::shared_ptr<WriteSessionFactoryBase> factory,
    TopicWriterWorkerSettings settings,
    bool run_loop
)
    : name_{std::move(name)},
      factory_{std::move(factory)},
      settings_{std::move(settings)},
      state_{TopicWriterWorkerState::kInitial},
      queue_{Queue::Create(settings_.max_incoming_msg_queue_size)},
      producer_(queue_->GetMultiProducer()),
      consumer_(queue_->GetConsumer()),
      event_queue_{YdbEventQueue::Create(settings_.max_ydb_control_event_queue_size)},
      event_producer_{event_queue_->GetProducer()},
      event_consumer_{event_queue_->GetConsumer()},
      ack_event_queue_{YdbEventQueue::Create(kMaxAckYdbEventQueueSize)},
      ack_event_producer_{ack_event_queue_->GetProducer()},
      ack_event_consumer_{ack_event_queue_->GetConsumer()}
{
    if (run_loop) {
        write_task_ = engine::CriticalAsyncNoTracing([this] { RunWriteTask(); });
    }
    LOG_INFO("[{}]: Created worker for topic[{}]", name_, settings_.topic);
}

TopicWriterWorker::~TopicWriterWorker() { write_task_.SyncCancel(); }

TopicWriteResult TopicWriterWorker::WriteMessage(std::string message, TopicWriterMetadata meta) {
    TopicWriterMessage
        msg{.message = std::move(message), .context = std::make_shared<impl::TopicWriterHandlingResult>()};
    auto context = msg.context;

    if (!producer_.PushNoblock(TopicWriterMessageItem{std::move(msg), std::move(meta)})) {
        ++stats_.messages_stats.resource_exhasted;
        return TopicWriteResult(TopicWriteStatus::kResourceExhausted);
    }

    ++stats_.messages_stats.received;
    return TopicWriteResult(utils::impl::InternalTag{}, TopicWriteStatus::kOk, std::move(context));
}

void DumpMetric(utils::statistics::Writer& writer, const TopicWriterWorker& worker) {
    writer.ValueWithLabels(worker.stats_, {{"worker", worker.name_}, {"topic", worker.settings_.topic}});

    {
        auto state_writer = writer["states"];
        auto state = worker.GetState();
        for (std::size_t i = 0; i < static_cast<std::size_t>(TopicWriterWorkerState::kDoNotUse); i++) {
            std::size_t value = (state == static_cast<TopicWriterWorkerState>(i)) ? 1 : 0;
            state_writer.ValueWithLabels(
                value,
                {{"state", ToStringView(state)}, {"worker", worker.name_}, {"topic", worker.settings_.topic}}
            );
        }
    }
}

NYdb::NTopic::TWriteSessionSettings TopicWriterWorker::CreateWriteSessionSettings() {
    NYdb::NTopic::TWriteSessionSettings::TEventHandlers handlers;
    handlers.ReadyToAcceptHandler([this](auto& event) {
        auto result = event_producer_.PushNoblock(event);
        if (result) {
            ++stats_.GetReadyToAcceptEventStats().received;
        } else {
            ++stats_.GetReadyToAcceptEventStats().resource_exhasted;
            LOG_DEBUG("[{}]: Unable to push ReadyToAccept event to queue", name_);
        }
    });

    handlers.AcksHandler([this](auto& event) {
        auto result = ack_event_producer_.PushNoblock(event);
        if (result) {
            ++stats_.GetAckEventStats().received;
        } else {
            ++stats_.GetAckEventStats().resource_exhasted;
            LOG_DEBUG("[{}]: Unable to push Ack event to queue", name_);
        }
    });

    handlers.SessionClosedHandler([this](auto& event) {
        auto result = event_producer_.PushNoblock(event);
        if (result) {
            ++stats_.GetSessionClosedEventStats().received;
        } else {
            ++stats_.GetSessionClosedEventStats().resource_exhasted;
        }
    });

    return NYdb::NTopic::TWriteSessionSettings().Path(settings_.topic).EventHandlers(handlers);
}

std::shared_ptr<NYdb::NTopic::IWriteSession> TopicWriterWorker::CreateWriteSession() {
    auto settings = CreateWriteSessionSettings();
    return factory_->CreateWriteSession(std::move(settings));
}

void TopicWriterWorker::CloseSessionGracefully() {
    if (write_session_.session) {
        ChangeState(TopicWriterWorkerState::kCloseInProgress);
        [[maybe_unused]] auto close_result =
            utils::AsyncHideSpan(engine::current_task::GetBlockingTaskProcessor(), [this] {
                return write_session_.session->Close(TDuration::MilliSeconds(kDefaultCloseTmo.count()));
            }).Get();

        if (!close_result) {
            LOG_ERROR("[{}]: Unable to close carefully session in time:[{}]", name_, kDefaultCloseTmo.count());
        } else {
            LOG_INFO("[{}]: Write session has been closed successfully on exit", name_);
        }

        // wait last event from session
        WaitCloseSessionEvent();
    }
}

void TopicWriterWorker::ChangeState(TopicWriterWorkerState new_state) {
    auto result = state_.exchange(new_state);
    if (result != new_state) {
        LOG_INFO("[{}]: Change state from {} to {}", name_, ToStringView(result), ToStringView(new_state));
    }
}

// Note: HandleYdbEvent() and HandleYdbEvent(TopicWriterYdbEventItem&) are two
// distinct overloads — there is no recursion here.
void TopicWriterWorker::HandleYdbEvent() {
    HandleCriticalEvent();
    HandleAckEvent();
}

void TopicWriterWorker::HandleCriticalEvent() {
    TopicWriterYdbEventItem item;
    // If there is no token, we must wait for it, since without it we cannot
    // process events, but we cannot block, since task cancellation may occur.

    // If the token is present, we will periodically check the message queue
    // and try to accept a message. If there is nothing there, we wait 100ms,
    // and check events again in case something appeared.
    // We need to handle session closure and ack receiving.
    auto deadline =
        write_session_.token.has_value()
            ? engine::Deadline::Passed()
            : engine::Deadline::FromDuration(kDefaultEventPopTmo);

    if (event_consumer_.Pop(item, deadline)) {
        stats_.event_handled++;
        HandleYdbEvent(item);
    }
}

void TopicWriterWorker::WaitCloseSessionEvent() {
    // we expect max 2 event on close: some left 'ready to accept' and close
    std::size_t max_attempts{2};

    bool closed = false;
    while (max_attempts > 0 && !closed) {
        TopicWriterYdbEventItem item;
        if (event_consumer_.Pop(item, engine::Deadline::FromDuration(kWaitCloseSessionEventOnExit))) {
            std::visit(
                utils::Overloaded{
                    [&closed, this](const NYdb::NTopic::TSessionClosedEvent&) {
                        LOG_INFO("[{}]: Final ClosedEvent received", name_);
                        closed = true;
                    },
                    [this](const NYdb::NTopic::TWriteSessionEvent::TReadyToAcceptEvent&) {
                        LOG_INFO("[{}]: Some left 'ready to accept' received. No actions", name_);
                    },
                    [this](const NYdb::NTopic::TWriteSessionEvent::TAcksEvent) {
                        UINVARIANT(
                            false,
                            fmt::format(
                                "[{}]: Unexpectedly TWriteSessionEvent::TAcksEvent "
                                "received on closed",
                                name_
                            )
                        );
                    }
                },
                item
            );
        } else {
            // kWaitCloseSessionEventOnExit is big enough,
            // if there is no message no actions anymore
            LOG_WARNING(
                "[{}]: There is no NYdb::NTopic::TSessionClosedEvent after wait. No "
                "actions",
                name_
            );
            break;
        }
        max_attempts--;
    }
}

void TopicWriterWorker::HandleAckEvent() {
    TopicWriterYdbEventItem item;
    std::size_t slice_read = 0;

    // At the moment we consider that handling of the acks is fast
    // and handle it slice by slice
    while (ack_event_consumer_.PopNoblock(item) && slice_read < kMaxAckEventReadSliceSize) {
        stats_.event_handled++;
        slice_read++;
        HandleYdbEvent(item);
    }
}

void TopicWriterWorker::HandleYdbEvent([[maybe_unused]] NYdb::NTopic::TWriteSessionEvent::TAcksEvent& event) {
    ++stats_.GetAckEventStats().handled;
    for (const auto& ack : event.Acks) {
        switch (ack.State) {
            case NYdb::NTopic::TWriteSessionEvent::TWriteAck::EEventState::EES_WRITTEN:
                stats_.event_acks_stats.last_acks = ack.SeqNo;
                stats_.event_acks_stats.written++;
                break;
            case NYdb::NTopic::TWriteSessionEvent::TWriteAck::EEventState::EES_ALREADY_WRITTEN:
                stats_.event_acks_stats.already_written++;
                break;
            case NYdb::NTopic::TWriteSessionEvent::TWriteAck::EEventState::EES_DISCARDED:
                stats_.event_acks_stats.discarded++;
                break;
            case NYdb::NTopic::TWriteSessionEvent::TWriteAck::EEventState::EES_WRITTEN_IN_TX:
                stats_.event_acks_stats.written_in_tx++;
                break;
        }
    }
}

void TopicWriterWorker::HandleYdbEvent(NYdb::NTopic::TWriteSessionEvent::TReadyToAcceptEvent& event) {
    ++stats_.GetReadyToAcceptEventStats().handled;
    write_session_.token = std::make_optional(std::move(event.ContinuationToken));
    ChangeState(TopicWriterWorkerState::kWorking);
}

void TopicWriterWorker::HandleYdbEvent([[maybe_unused]] NYdb::NTopic::TSessionClosedEvent& event) {
    // We do not expect this event often, so let's print the debug
    // information to log
    LOG_INFO() << "ReceivedTSessionClosedEvent:::" << event.DebugString();
    ++stats_.GetSessionClosedEventStats().handled;
    CleanWriteSession();
}

void TopicWriterWorker::HandleYdbEvent(TopicWriterYdbEventItem& event) {
    std::visit([this](auto& event) { this->HandleYdbEvent(event); }, event);
}

void TopicWriterWorker::CleanWriteSession() {
    bool recreate = (GetState() == TopicWriterWorkerState::kWorking);
    write_session_.CleanSessionData(recreate);
    ChangeState(TopicWriterWorkerState::kDisconnected);
}

void TopicWriterWorker::ReInitWriteSession() {
    auto state = GetState();

    bool status =
        ((state == TopicWriterWorkerState::kDisconnected) || (state == TopicWriterWorkerState::kInitial)) &&
        (write_session_.session == nullptr);
    if (status) {
        auto delay = write_session_.GetDelayForError();
        LOG_DEBUG(
            "Session disconnected, sleep [{}ms] for next actions",
            std::chrono::duration_cast<std::chrono::milliseconds>(delay.TimeLeft()).count()
        );
        engine::InterruptibleSleepFor(delay.TimeLeft());

        write_session_.session = CreateWriteSession();
        write_session_.token = std::nullopt;
    }
}

void TopicWriterWorker::HandleMessage() {
    TopicWriterMessageItem item;
    if (IsReady() && write_session_.token.has_value()) {
        if (consumer_.Pop(item, engine::Deadline::FromDuration(kDefaultMessagePopTmo))) {
            ++stats_.messages_stats.handled;
            try {
                NYdb::NTopic::TWriteMessage message(item.message.message);
                if (!item.metadata.empty()) {
                    std::vector<std::pair<std::string, std::string>> meta;
                    for (const auto& elem : item.metadata) {
                        meta.push_back(elem);
                    }

                    message.MessageMeta(meta);
                }
                write_session_.session->Write(std::move(write_session_.token.value()), std::move(message));
                write_session_.token = std::nullopt;
            } catch (std::exception& e) {
                LOG_LIMITED_ERROR("[{}]:Error at message write:[{}]", name_, e.what());
                ++stats_.write_issues;
            }
            TESTPOINT("sent-data-to-ydb-topic", {});
        }
    }
}

void TopicWriterWorker::TestHandlers() { WriteTask(); }

void TopicWriterWorker::WriteTask() {
    HandleYdbEvent();
    ReInitWriteSession();  // check if we need to create new session
    HandleMessage();
}

void TopicWriterWorker::RunWriteTask() {
    LOG_INFO("[{}]: Start YDB topic write task", name_);

    while (!engine::current_task::ShouldCancel()) {
        try {
            WriteTask();
        } catch (std::exception& e) {
            ++stats_.main_cycle_issue;
            LOG_ERROR("[{}]: Unexpected exception received. Exc:{}", name_, e.what());
        }
        engine::Yield();
    }

    // cleanup session on exit
    try {
        CloseSessionGracefully();

    } catch (std::exception& e) {
        LOG_ERROR("[{}]: Received exception on session close: {}", name_, e.what());
        ++stats_.on_close_issue;
    }
    LOG_INFO("[{}]: End YDB write task", name_);
}

}  // namespace ydb::impl

USERVER_NAMESPACE_END
