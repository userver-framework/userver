#include <userver/ydb/federated_topic.hpp>

#include <userver/engine/async.hpp>
#include <userver/ydb/impl/cast.hpp>

#include <ydb/impl/config.hpp>
#include <ydb/impl/driver.hpp>
#include <ydb/impl/future.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb {

FederatedTopicReadSession::FederatedTopicReadSession(std::shared_ptr<
                                                     NYdb::NFederatedTopic::IFederatedReadSession> read_session)
    : read_session_(std::move(read_session))
{
    UASSERT(read_session_);
}

std::vector<NYdb::NFederatedTopic::TReadSessionEvent::TEvent> FederatedTopicReadSession::GetEvents(
    std::optional<std::size_t> max_events_count,
    size_t max_size_bytes
) {
    impl::GetFutureValue(read_session_->WaitEvent());
    return read_session_->GetEvents(false, max_events_count, max_size_bytes);
}

bool FederatedTopicReadSession::Close(std::chrono::milliseconds timeout) { return read_session_->Close(timeout); }

NYdb::NFederatedTopic::IFederatedReadSession& FederatedTopicReadSession::GetNativeTopicReadSession()
    USERVER_IMPL_LIFETIME_BOUND {
    UASSERT(read_session_);
    return *read_session_;
}

FederatedTopicClient::FederatedTopicClient(
    std::shared_ptr<impl::Driver> driver,
    [[maybe_unused]] impl::TopicSettings settings
)
    : driver_{std::move(driver)},
      // TODO: use shared thread pool executors from driver?
      // These are the default thread pool executors used by the ydb-cpp-sdk.
      compression_executor_{NYdb::CreateThreadPoolExecutor(2)},
      handlers_executor_{NYdb::CreateThreadPoolExecutor(1)},
      topic_client_{
          std::in_place,
          driver_->GetNativeDriver(),
          NYdb::NFederatedTopic::TFederatedTopicClientSettings()
              .DefaultCompressionExecutor(compression_executor_)
              .DefaultHandlersExecutor(handlers_executor_)
      }
{}

FederatedTopicClient::~FederatedTopicClient() {
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

FederatedTopicReadSession FederatedTopicClient::CreateReadSession(
    const NYdb::NFederatedTopic::TFederatedReadSessionSettings& settings
) {
    return FederatedTopicReadSession{topic_client_->CreateReadSession(settings)};
}

NYdb::NFederatedTopic::TFederatedTopicClient& FederatedTopicClient::GetNativeTopicClient() USERVER_IMPL_LIFETIME_BOUND {
    UASSERT(topic_client_);
    return *topic_client_;
}

}  // namespace ydb

USERVER_NAMESPACE_END
