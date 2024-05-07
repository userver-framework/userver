#include <userver/ydb/topic.hpp>

#include <userver/ydb/impl/cast.hpp>

#include <ydb/impl/config.hpp>
#include <ydb/impl/driver.hpp>
#include <ydb/impl/future.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb {

TopicReadSession::TopicReadSession(
    std::shared_ptr<NYdb::NTopic::IReadSession> read_session)
    : read_session_(std::move(read_session)) {
  UASSERT(read_session_);
}

std::vector<NYdb::NTopic::TReadSessionEvent::TEvent>
TopicReadSession::GetEvents(std::optional<std::size_t> max_events_count) {
  impl::GetFutureValue(read_session_->WaitEvent());
  if (max_events_count.has_value()) {
    return read_session_->GetEvents(false, *max_events_count);
  } else {
    return read_session_->GetEvents(false);
  }
}

bool TopicReadSession::Close(std::chrono::milliseconds timeout) {
  return read_session_->Close(timeout);
}

std::shared_ptr<NYdb::NTopic::IReadSession>
TopicReadSession::GetNativeTopicReadSession() {
  return read_session_;
}

TopicClient::TopicClient(std::shared_ptr<impl::Driver> driver,
                         [[maybe_unused]] impl::TopicSettings settings)
    : driver_{std::move(driver)}, topic_client_{driver_->GetNativeDriver()} {}

TopicClient::~TopicClient() = default;

void TopicClient::AlterTopic(
    const std::string& path,
    const NYdb::NTopic::TAlterTopicSettings& settings) {
  impl::GetFutureValueChecked(
      topic_client_.AlterTopic(impl::ToString(path), settings), "AlterTopic");
}

NYdb::NTopic::TDescribeTopicResult TopicClient::DescribeTopic(
    const std::string& path) {
  return impl::GetFutureValueChecked(
      topic_client_.DescribeTopic(impl::ToString(path)), "DescribeTopic");
}

TopicReadSession TopicClient::CreateReadSession(
    const NYdb::NTopic::TReadSessionSettings& settings) {
  return TopicReadSession{topic_client_.CreateReadSession(settings)};
}

NYdb::NTopic::TTopicClient& TopicClient::GetNativeTopicClient() {
  return topic_client_;
}

}  // namespace ydb

USERVER_NAMESPACE_END
