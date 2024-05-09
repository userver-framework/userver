#pragma once

#include <optional>
#include <vector>

#include <userver/engine/deadline.hpp>
#include <userver/kafka/message.hpp>

#include <kafka/impl/error_buffer.hpp>
#include <kafka/impl/opaque.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka::impl {

class Configuration;
struct TopicStats;

/// @brief Consumer implementation based on `librdkafka`.
/// @warning All methods calls the `librdkafka` functions that very often uses
/// pthread mutexes. Hence, all methods must not be called in main task
/// processor
class ConsumerImpl final {
  using MessageBatch = std::vector<Message>;

 public:
  explicit ConsumerImpl(std::unique_ptr<Configuration> configuration);

  ~ConsumerImpl();

  /// @brief Schedules the @param topics subscription.
  void Subscribe(const std::vector<std::string>& topics);

  /// @brief Revokes all subscribed topics partitions and leaves the consumer
  /// group.
  /// @note Blocks until consumer successfully closed
  /// @warning Blocks forever if polled messages are not destroyed
  void LeaveGroup();

  /// @brief Closes the consumer and subscribes for the @param topics.
  void Resubscribe(const std::vector<std::string>& topics);

  /// @brief Synchronously commits the current assignment offsets.
  void Commit();

  /// @brief Schedules the committment task.
  void AsyncCommit();

  /// @brief Polls the message until @param deadline is reached.
  /// If no message polled, returns `std::nullopt`
  /// @note Must be called periodically to maintain consumer group membership
  std::optional<Message> PollMessage(engine::Deadline deadline);

  /// @brief Effectively calles `PollMessage` until @param deadline is reached
  /// and no more than @param max_batch_size messages polled.
  MessageBatch PollBatch(std::size_t max_batch_size, engine::Deadline deadline);

  const Stats& GetStats() const;

  void AccountMessageProccessingSucceeded(const Message& message);
  void AccountMessageBatchProccessingSucceeded(const MessageBatch& batch);
  void AccountMessageProccessingFailed(const Message& message);
  void AccountMessageBatchProcessingFailed(const MessageBatch& batch);

 private:
  std::shared_ptr<TopicStats> GetTopicStats(const std::string& topic);

  void AccountPolledMessageStat(const Message& polled_message);

 private:
  Opaque opaque_;

  // consumer is recreated on each `Resubscribe`, though err_buf for that reason
  // is cached here
  ErrorBuffer err_buf_{};

  class ConfHolder;
  std::unique_ptr<ConfHolder> conf_;

  class ConsumerHolder;
  std::unique_ptr<ConsumerHolder> consumer_{nullptr};
};

}  // namespace kafka::impl

USERVER_NAMESPACE_END
