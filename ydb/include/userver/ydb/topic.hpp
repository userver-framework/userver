#pragma once

#include <chrono>
#include <memory>
#include <string>

#include <ydb-cpp-sdk/client/topic/topic.h>

USERVER_NAMESPACE_BEGIN

namespace ydb {

namespace impl {
class Driver;
struct TopicSettings;
}  // namespace impl

class TopicReadSession final {
 public:
  /// @cond
  // For internal use only.
  explicit TopicReadSession(
      std::shared_ptr<NYdb::NTopic::IReadSession> read_session);
  /// @endcond

  std::vector<NYdb::NTopic::TReadSessionEvent::TEvent> GetEvents(
      std::optional<std::size_t> max_events_count = {});

  bool Close(std::chrono::milliseconds timeout);

  std::shared_ptr<NYdb::NTopic::IReadSession> GetNativeTopicReadSession();

 private:
  std::shared_ptr<NYdb::NTopic::IReadSession> read_session_;
};

class TopicClient final {
 public:
  /// @cond
  // For internal use only.
  TopicClient(std::shared_ptr<impl::Driver> driver,
              impl::TopicSettings settings);
  /// @endcond

  ~TopicClient();

  void AlterTopic(const std::string& path,
                  const NYdb::NTopic::TAlterTopicSettings& settings);

  NYdb::NTopic::TDescribeTopicResult DescribeTopic(const std::string& path);

  TopicReadSession CreateReadSession(
      const NYdb::NTopic::TReadSessionSettings& settings);

  NYdb::NTopic::TTopicClient& GetNativeTopicClient();

 private:
  std::shared_ptr<impl::Driver> driver_;
  NYdb::NTopic::TTopicClient topic_client_;
};

}  // namespace ydb

USERVER_NAMESPACE_END
