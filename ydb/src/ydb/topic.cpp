#include <userver/ydb/topic.hpp>

#include <userver/engine/async.hpp>
#include <userver/ydb/impl/cast.hpp>

#include <ydb/impl/config.hpp>
#include <ydb/impl/driver.hpp>
#include <ydb/impl/future.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb {

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

NYdb::NTopic::TTopicClient& TopicClient::GetNativeTopicClient() USERVER_IMPL_LIFETIME_BOUND {
    UASSERT(topic_client_);
    return *topic_client_;
}

}  // namespace ydb

USERVER_NAMESPACE_END
