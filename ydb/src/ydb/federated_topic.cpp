#include <userver/ydb/federated_topic.hpp>

#include <userver/engine/async.hpp>
#include <userver/ydb/impl/cast.hpp>

#include <ydb/impl/config.hpp>
#include <ydb/impl/driver.hpp>
#include <ydb/impl/future.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb {

FederatedTopicReadSession::FederatedTopicReadSession(
    std::shared_ptr<NYdb::NFederatedTopic::IFederatedReadSession> read_session
)
    : read_session_(std::move(read_session)) {
    UASSERT(read_session_);
}

std::vector<NYdb::NFederatedTopic::TReadSessionEvent::TEvent>
FederatedTopicReadSession::GetEvents(std::optional<std::size_t> max_events_count, size_t max_size_bytes) {
    impl::GetFutureValue(read_session_->WaitEvent());
    return read_session_->GetEvents(false, max_events_count, max_size_bytes);
}

bool FederatedTopicReadSession::Close(std::chrono::milliseconds timeout) { return read_session_->Close(timeout); }

std::shared_ptr<NYdb::NFederatedTopic::IFederatedReadSession> FederatedTopicReadSession::GetNativeTopicReadSession() {
    return read_session_;
}

FederatedTopicClient::FederatedTopicClient(
    std::shared_ptr<impl::Driver> driver,
    [[maybe_unused]] impl::TopicSettings settings
)
    : driver_{std::move(driver)}, topic_client_{driver_->GetNativeDriver()} {}

FederatedTopicClient::~FederatedTopicClient() = default;

FederatedTopicReadSession FederatedTopicClient::CreateReadSession(
    const NYdb::NFederatedTopic::TFederatedReadSessionSettings& settings
) {
    return FederatedTopicReadSession{topic_client_.CreateReadSession(settings)};
}

NYdb::NFederatedTopic::TFederatedTopicClient& FederatedTopicClient::GetNativeTopicClient() { return topic_client_; }

}  // namespace ydb

USERVER_NAMESPACE_END
