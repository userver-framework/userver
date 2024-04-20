#pragma once

#include <queue>

#include <userver/kafka/stats.hpp>

#include <userver/engine/task/task_with_result.hpp>
#include <userver/utils/statistics/entry.hpp>
#include <userver/utils/statistics/storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace cppkafka {

class Configuration;
class Message;
class Consumer;

}  // namespace cppkafka

namespace kafka {

struct MessagePolled {
  std::string key;
  std::string payload;
  std::string topic;
  std::optional<std::chrono::milliseconds> timestamp = std::nullopt;
  int partition{};
  int64_t offset{};

  MessagePolled() = default;

  MessagePolled(cppkafka::Message&& message);
};

class Consumer;

/// Must be placed as one of the last fields in the consumer component.
/// Make sure to add a comment before the field:
///
/// @code
/// // Subscription must be the last field! Add new fields above this comment.
/// @endcode
class ConsumerScope final {
 public:
  using Callback = std::function<void(std::vector<MessagePolled>&&)>;

  ConsumerScope(ConsumerScope&&) noexcept = delete;
  ConsumerScope& operator=(ConsumerScope&&) noexcept = delete;
  ~ConsumerScope();

  /// @note Each message pack must be committed using ConsumerScope::Commit.
  void Start(Callback callback);

  /// Called in the destructor of ConsumerScope automatically. Can be called in
  /// the beginning of your destructor if some other actions in that destructor
  /// prevent the callback from functioning correctly.
  void Stop() noexcept;

  /// Each message pack must be committed after it's processed (either with a
  /// satisfactory or unsatisfactory result), otherwise the same messages will
  /// come back over and over again.
  ///
  /// @note Instead of calling `AsyncCommit` manually, consider setting
  /// `enable_auto_commit: true` in the static config.
  void AsyncCommit();

 private:
  friend class Consumer;

  explicit ConsumerScope(Consumer& consumer) noexcept;

  Consumer& consumer_;
};

class Consumer final {
 public:
  Consumer(std::unique_ptr<cppkafka::Configuration> config,
           const std::string& component_name,
           const std::vector<std::string>& topics, std::size_t max_batch_size,
           engine::TaskProcessor& consumer_task_processor,
           engine::TaskProcessor& main_task_processor, bool is_testsuite_mode,
           utils::statistics::Storage& storage);

  ~Consumer();

  ConsumerScope MakeConsumerScope();

  void PushTestMessage(MessagePolled&& message);

 private:
  friend class ConsumerScope;

  void StartMessageProcessing(ConsumerScope::Callback callback);
  void Stop() noexcept;
  void AsyncCommit();

  std::vector<MessagePolled> GetPolledMessages();
  void Init();
  void GetAssigment();

 private:
  std::atomic_flag started_processing_{false};

  /// @note `stats_` must be initialized before `consumer_`
  Stats stats_;

  const std::string& component_name_;
  const std::vector<std::string> topics_;
  const std::size_t max_batch_size_;
  std::unique_ptr<cppkafka::Consumer> consumer_;
  engine::Task poll_task_;
  engine::TaskProcessor& consumer_task_processor_;
  engine::TaskProcessor& main_task_processor_;

  /// for testsuite
  std::queue<MessagePolled> tests_messages_;
  const bool is_testsuite_mode_;

  /// @note Subscriptions must be the last fields! Add new fields above this
  /// comment.
  utils::statistics::Entry statistics_holder_;
};

}  // namespace kafka

USERVER_NAMESPACE_END
