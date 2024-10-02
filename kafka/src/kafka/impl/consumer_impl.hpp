#pragma once

#include <optional>
#include <vector>

#include <librdkafka/rdkafka.h>

#include <userver/engine/deadline.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/kafka/impl/holders.hpp>
#include <userver/kafka/message.hpp>

#include <kafka/impl/holders_aliases.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka::impl {

struct Stats;
struct TopicStats;

/// @brief Consumer implementation based on `librdkafka`.
/// @warning All methods calls the `librdkafka` functions that very often uses
/// pthread mutexes. Hence, all methods must not be called in main task
/// processor
class ConsumerImpl final {
  using MessageBatch = std::vector<Message>;

 public:
  ConsumerImpl(const std::string& name, const ConfHolder& conf,
               const std::vector<std::string>& topics, Stats& stats);

  const Stats& GetStats() const;

  /// @brief Synchronously commits the current assignment offsets.
  void Commit();

  /// @brief Schedules the commitment task.
  void AsyncCommit();

  /// @brief Effectively calls `PollMessage` until `deadline` is reached
  /// and no more than `max_batch_size` messages polled.
  MessageBatch PollBatch(std::size_t max_batch_size, engine::Deadline deadline);

  void AccountMessageProcessingSucceeded(const Message& message);
  void AccountMessageBatchProcessingSucceeded(const MessageBatch& batch);
  void AccountMessageProcessingFailed(const Message& message);
  void AccountMessageBatchProcessingFailed(const MessageBatch& batch);

  void EventCallback();

  /// @brief Revokes all subscribed topics partitions and leaves the consumer
  /// group.
  /// @note Blocks until consumer successfully closed
  /// @warning Blocks forever if polled messages are not destroyed
  /// @warning May throw in testsuite because calls testpoints
  void StopConsuming();

 private:
  /// @brief Schedules the `topics_` subscription.
  void StartConsuming(const std::vector<std::string>& topics);

  /// @brief Try to poll the message until `deadline` is reached.
  /// If no message polled until the deadline, returns
  /// `std::nullopt`.
  /// @note Must be called periodically to maintain consumer group membership
  std::optional<Message> PollMessage(engine::Deadline deadline);

  /// @brief Poll a delivery or error event from producer's queue.
  EventHolder PollEvent();

  /// @brief Retrieves a message from the event, accounts statistics.
  /// @returns std::nullopt if event's messages contains an error.
  std::optional<Message> TakeEventMessage(EventHolder&& event_holder);

  /// @brief Call a corresponding callback for the event data depends on its
  /// type. Must be called for all events except FETCH.
  void DispatchEvent(const EventHolder& event_holder);

  /// @brief Callback called on error in `librdkafka` work.
  void ErrorCallback(rd_kafka_resp_err_t error, const char* reason,
                     bool is_fatal);

  /// @brief Callback called on debug `librdkafka` messages.
  void LogCallback(const char* facility, const char* message, int log_level);

  /// @brief Callback that is called on each group join/leave and topic
  /// partition update. Used as a dispatcher of rebalance events.
  void RebalanceCallback(rd_kafka_resp_err_t err,
                         const rd_kafka_topic_partition_list_s* partitions);

  /// @brief Assigns (subscribes) the `partitions` list to the current
  /// consumer.
  void AssignPartitions(const rd_kafka_topic_partition_list_s* partitions);

  /// @brief Revokes `partitions` from the current consumer.
  void RevokePartitions(const rd_kafka_topic_partition_list_s* partitions);

  /// @brief Callback which is called after succeeded/failed commit.
  /// Currently, used for logging purposes.
  void OffsetCommitCallback(
      rd_kafka_resp_err_t err,
      const rd_kafka_topic_partition_list_s* committed_offsets);

  std::shared_ptr<TopicStats> GetTopicStats(const std::string& topic);

  void AccountPolledMessageStat(const Message& polled_message);

 private:
  const std::string& name_;
  Stats& stats_;

  engine::SingleConsumerEvent queue_became_non_empty_event_;

  ConsumerHolder consumer_;
};

}  // namespace kafka::impl

USERVER_NAMESPACE_END
