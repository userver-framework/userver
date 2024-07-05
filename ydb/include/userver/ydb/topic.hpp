#pragma once

/// @file userver/ydb/topic.hpp
/// @brief YDB Topic client

#include <chrono>
#include <memory>
#include <string>

#include <ydb-cpp-sdk/client/topic/client.h>

USERVER_NAMESPACE_BEGIN

namespace ydb {

namespace impl {
class Driver;
struct TopicSettings;
}  // namespace impl

/// @brief Read session used to connect to one or more topics for reading
///
/// @see https://ydb.tech/docs/en/reference/ydb-sdk/topic#reading
///
/// ## Example usage:
///
/// @snippet userver/samples/ydb_service/components/topic_reader.hpp  Sample
/// Topic reader
class TopicReadSession final {
 public:
  /// @cond
  // For internal use only.
  explicit TopicReadSession(
      std::shared_ptr<NYdb::NTopic::IReadSession> read_session);
  /// @endcond

  /// @brief Get read session events
  ///
  /// Waits until event occurs
  /// @param max_events_count maximum events count in batch
  /// if not specified, read session chooses event batch size automatically
  std::vector<NYdb::NTopic::TReadSessionEvent::TEvent> GetEvents(
      std::optional<std::size_t> max_events_count = {});

  /// @brief Close read session
  ///
  /// Waits for all commit acknowledgments to arrive.
  /// Force close after timeout
  bool Close(std::chrono::milliseconds timeout);

  /// Get native read session
  /// @warning Use with care! Facilities from
  /// `<core/include/userver/drivers/subscribable_futures.hpp>` can help with
  /// non-blocking wait operations.
  std::shared_ptr<NYdb::NTopic::IReadSession> GetNativeTopicReadSession();

 private:
  std::shared_ptr<NYdb::NTopic::IReadSession> read_session_;
};

/// @ingroup userver_clients
///
/// @brief YDB Topic Client
///
/// @see https://ydb.tech/docs/en/concepts/topic
class TopicClient final {
 public:
  /// @cond
  // For internal use only.
  TopicClient(std::shared_ptr<impl::Driver> driver,
              impl::TopicSettings settings);
  /// @endcond

  ~TopicClient();

  /// Alter topic
  void AlterTopic(const std::string& path,
                  const NYdb::NTopic::TAlterTopicSettings& settings);

  /// Describe topic
  NYdb::NTopic::TDescribeTopicResult DescribeTopic(const std::string& path);

  /// Create read session
  TopicReadSession CreateReadSession(
      const NYdb::NTopic::TReadSessionSettings& settings);

  /// Get native topic client
  /// @warning Use with care! Facilities from
  /// `<core/include/userver/drivers/subscribable_futures.hpp>` can help with
  /// non-blocking wait operations.
  NYdb::NTopic::TTopicClient& GetNativeTopicClient();

 private:
  std::shared_ptr<impl::Driver> driver_;
  NYdb::NTopic::TTopicClient topic_client_;
};

}  // namespace ydb

USERVER_NAMESPACE_END
