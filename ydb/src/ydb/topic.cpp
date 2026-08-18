#include <userver/ydb/topic.hpp>

#include <variant>

#include <userver/engine/async.hpp>
#include <userver/engine/task/current_task.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/async.hpp>
#include <userver/ydb/impl/cast.hpp>

#include <ydb/impl/config.hpp>
#include <ydb/impl/driver.hpp>
#include <ydb/impl/future.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb {

namespace {

void ResetSimpleWriteSessionHandlers(NYdb::NTopic::TWriteSessionSettings::TEventHandlers& handlers) {
    // Keep this decomposition exhaustive so that adding a field in the YDB SDK
    // requires an explicit decision here.
    auto& [acks_handler, ready_to_accept_handler, session_closed_handler, common_handler, handlers_executor] = handlers;

    if (acks_handler) {
        LOG_WARNING("TopicSimpleWriteSession cannot use AcksHandler, resetting");
    } else if (ready_to_accept_handler) {
        LOG_WARNING("TopicSimpleWriteSession cannot use ReadyToAcceptHandler, resetting");
    } else if (session_closed_handler) {
        LOG_WARNING("TopicSimpleWriteSession cannot use SessionClosedHandler, resetting");
    } else if (common_handler) {
        LOG_WARNING("TopicSimpleWriteSession cannot use CommonHandler, resetting");
    } else {
        return;
    }

    auto saved_handlers_executor = std::move(handlers_executor);
    handlers = {};
    handlers.HandlersExecutor_ = std::move(saved_handlers_executor);
}

TInstant ToYdbInstant(std::chrono::system_clock::time_point time_point) {
    return TInstant::MicroSeconds(std::chrono::duration_cast<std::chrono::microseconds>(time_point.time_since_epoch())
                                      .count());
}

}  // namespace

TopicReadSession::TopicReadSession(std::shared_ptr<NYdb::NTopic::IReadSession> read_session)
    : read_session_(std::move(read_session))
{
    UASSERT(read_session_);
}

std::vector<NYdb::NTopic::TReadSessionEvent::TEvent> TopicReadSession::GetEvents(
    std::optional<std::size_t> max_events_count,
    size_t max_size_bytes
) {
    impl::GetFutureValue(read_session_->WaitEvent());
    return read_session_->GetEvents(false, max_events_count, max_size_bytes);
}

std::vector<NYdb::NTopic::TReadSessionEvent::TEvent> TopicReadSession::GetEvents(
    const NYdb::NTopic::TReadSessionGetEventSettings& settings
) {
    impl::GetFutureValue(read_session_->WaitEvent());
    return read_session_->GetEvents(settings);
}

bool TopicReadSession::Close(std::chrono::milliseconds timeout) { return read_session_->Close(timeout); }

NYdb::NTopic::IReadSession& TopicReadSession::GetNativeTopicReadSession() USERVER_IMPL_LIFETIME_BOUND {
    UASSERT(read_session_);
    return *read_session_;
}

TopicWriteSession::TopicWriteSession(std::shared_ptr<NYdb::NTopic::IWriteSession> write_session)
    : write_session_(std::move(write_session))
{
    UASSERT(write_session_);
}

NYdb::NTopic::TWriteSessionEvent::TEvent TopicWriteSession::GetEvent() {
    while (true) {
        impl::GetFutureValue(write_session_->WaitEvent());
        if (auto event = write_session_->GetEvent(/*block=*/false)) {
            return std::move(*event);
        }
        // In case of races between multiple GetEvent() calls, we may need to retry awaiting the event.
    }
}

std::optional<NYdb::NTopic::TWriteSessionEvent::TEvent> TopicWriteSession::TryGetEvent() {
    return write_session_->GetEvent(/*block=*/false);
}

void TopicWriteSession::Write(NYdb::NTopic::TContinuationToken&& token, NYdb::NTopic::TWriteMessage&& message) {
    write_session_->Write(std::move(token), std::move(message));
}

bool TopicWriteSession::Close(std::chrono::milliseconds timeout) { return write_session_->Close(timeout); }

NYdb::NTopic::IWriteSession& TopicWriteSession::GetNativeTopicWriteSession() USERVER_IMPL_LIFETIME_BOUND {
    UASSERT(write_session_);
    return *write_session_;
}

TopicSimpleWriteSession::TopicSimpleWriteSession(std::shared_ptr<NYdb::NTopic::IWriteSession> write_session)
    : write_session_(std::move(write_session))
{
    UASSERT(write_session_);
}

TopicSimpleWriteSession::TopicSimpleWriteSession(TopicSimpleWriteSession&& other) noexcept
    : write_session_(std::move(other.write_session_)),
      closed_(other.closed_.exchange(true))
{}

TopicSimpleWriteSession& TopicSimpleWriteSession::operator=(TopicSimpleWriteSession&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    write_session_ = std::move(other.write_session_);
    closed_.store(other.closed_.exchange(true));
    return *this;
}

bool TopicSimpleWriteSession::Write(
    NYdb::NTopic::TWriteMessage&& message,
    NYdb::TTransactionBase* tx,
    engine::Deadline deadline
) {
    auto continuation_token = WaitForToken(deadline);
    if (!continuation_token.has_value()) {
        return false;
    }

    write_session_->Write(std::move(*continuation_token), std::move(message), tx);
    return true;
}

bool TopicSimpleWriteSession::Write(
    std::string_view data,
    std::optional<std::uint64_t> seq_no,
    std::optional<std::chrono::system_clock::time_point> create_timestamp,
    engine::Deadline deadline
) {
    NYdb::NTopic::TWriteMessage message{data};
    if (seq_no.has_value()) {
        message.SeqNo(seq_no);
    }
    if (create_timestamp.has_value()) {
        message.CreateTimestamp(ToYdbInstant(*create_timestamp));
    }
    return Write(std::move(message), nullptr, deadline);
}

std::uint64_t TopicSimpleWriteSession::GetInitSeqNo(engine::Deadline deadline) {
    return impl::GetFutureValue(write_session_->GetInitSeqNo(), deadline);
}

bool TopicSimpleWriteSession::Close(std::chrono::milliseconds timeout) {
    if (!closed_.exchange(true)) {
        return utils::AsyncHideSpan(
                   engine::current_task::GetBlockingTaskProcessor(),
                   [this, timeout] { return write_session_->Close(timeout); }
        ).Get();
    }
    return true;
}

bool TopicSimpleWriteSession::IsAlive() const noexcept { return !closed_.load(); }

NYdb::NTopic::IWriteSession& TopicSimpleWriteSession::GetNativeTopicWriteSession() USERVER_IMPL_LIFETIME_BOUND {
    UASSERT(write_session_);
    return *write_session_;
}

std::optional<NYdb::NTopic::TContinuationToken> TopicSimpleWriteSession::WaitForToken(engine::Deadline deadline) {
    while (IsAlive()) {
        try {
            auto wait_event = write_session_->WaitEvent();
            impl::WaitForFuture(wait_event, deadline);
        } catch (const DeadlineExceededError&) {
            return std::nullopt;
        }

        std::optional<NYdb::NTopic::TContinuationToken> token;
        for (auto& event : write_session_->GetEvents(/*block=*/false, std::nullopt)) {
            if (auto* ready_event = std::get_if<NYdb::NTopic::TWriteSessionEvent::TReadyToAcceptEvent>(&event)) {
                UINVARIANT(!token.has_value(), "Multiple YDB topic continuation tokens in one event batch");
                token = std::move(ready_event->ContinuationToken);
            } else if (std::get_if<NYdb::NTopic::TSessionClosedEvent>(&event)) {
                // The native session is already closed; only mirror its state.
                closed_.store(true);
                return std::nullopt;
            } else if (std::get_if<NYdb::NTopic::TWriteSessionEvent::TAcksEvent>(&event)) {
                // TopicSimpleWriteSession intentionally does not expose acknowledgments.
            } else {
                UINVARIANT(false, "Unexpected YDB topic write session event");
            }
        }

        if (token.has_value()) {
            return token;
        }
        if (deadline.IsReached()) {
            return std::nullopt;
        }
    }

    return std::nullopt;
}

TopicProducer::TopicProducer(std::shared_ptr<NYdb::NTopic::IProducer> producer)
    : producer_(std::move(producer))
{
    UASSERT(producer_);
}

NYdb::NTopic::TWriteResult TopicProducer::Write(NYdb::NTopic::TWriteMessage&& message) {
    return producer_->Write(std::move(message));
}

NYdb::NTopic::TFlushResult TopicProducer::Flush(engine::Deadline deadline) {
    return impl::GetFutureValue(producer_->Flush(), deadline);
}

NYdb::NTopic::TCloseResult TopicProducer::Close(std::chrono::milliseconds timeout) { return producer_->Close(timeout); }

NYdb::NTopic::IProducer& TopicProducer::GetNativeTopicProducer() USERVER_IMPL_LIFETIME_BOUND {
    UASSERT(producer_);
    return *producer_;
}

TopicClient::TopicClient(std::shared_ptr<impl::Driver> driver, [[maybe_unused]] impl::TopicSettings settings)
    : driver_{std::move(driver)},
      // TODO: use shared thread pool executors from driver?
      // These are the default thread pool executors used by the ydb-cpp-sdk.
      compression_executor_{NYdb::CreateThreadPoolExecutor(2)},
      handlers_executor_{NYdb::CreateThreadPoolExecutor(1)},
      topic_client_{
          std::in_place,
          driver_->GetNativeDriver(),
          NYdb::NTopic::TTopicClientSettings()
              .DefaultCompressionExecutor(compression_executor_)
              .DefaultHandlersExecutor(handlers_executor_)
      }
{}

TopicClient::~TopicClient() {
    // Destroy the native client first so sessions flush and no longer post to
    // the executors; then join executor threads.
    topic_client_.reset();
    // Joins background threads of the compression and handlers thread pools.
    // Without this, posted tasks may still be running while the
    // ydb-cpp-sdk's globals (e.g. TCodecMap singleton in codecs.h) are torn
    // down during atexit, leading to a use-after-destroy SEGV at process
    // shutdown.
    compression_executor_->Stop();
    handlers_executor_->Stop();
}

void TopicClient::AlterTopic(const std::string& path, const NYdb::NTopic::TAlterTopicSettings& settings) {
    impl::GetFutureValueChecked(topic_client_->AlterTopic(impl::ToString(path), settings), "AlterTopic");
}

NYdb::NTopic::TDescribeTopicResult TopicClient::DescribeTopic(const std::string& path) {
    return impl::GetFutureValueChecked(topic_client_->DescribeTopic(impl::ToString(path)), "DescribeTopic");
}

TopicReadSession TopicClient::CreateReadSession(const NYdb::NTopic::TReadSessionSettings& settings) {
    return TopicReadSession{topic_client_->CreateReadSession(settings)};
}

TopicWriteSession TopicClient::CreateWriteSession(const NYdb::NTopic::TWriteSessionSettings& settings) {
    return TopicWriteSession{topic_client_->CreateWriteSession(settings)};
}

TopicSimpleWriteSession TopicClient::CreateSimpleWriteSession(const NYdb::NTopic::TWriteSessionSettings& settings) {
    auto simple_write_session_settings = settings;
    ResetSimpleWriteSessionHandlers(simple_write_session_settings.EventHandlers_);
    return TopicSimpleWriteSession{topic_client_->CreateWriteSession(simple_write_session_settings)};
}

TopicProducer TopicClient::CreateProducer(const TopicProducerSettings& settings, std::uint64_t max_memory_usage_bytes) {
    auto native_settings = static_cast<const NYdb::NTopic::TProducerSettings&>(settings);
    native_settings.MaxMemoryUsage(max_memory_usage_bytes);
    native_settings.MaxBlockTimeout(TDuration::Zero());
    native_settings.AsyncExecutionMode(true);
    return TopicProducer{topic_client_->CreateProducer(native_settings)};
}

NYdb::NTopic::TTopicClient& TopicClient::GetNativeTopicClient() USERVER_IMPL_LIFETIME_BOUND {
    UASSERT(topic_client_);
    return *topic_client_;
}

}  // namespace ydb

USERVER_NAMESPACE_END
