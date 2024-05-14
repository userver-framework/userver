#pragma once

#include <optional>
#include <vector>

#include <userver/engine/deadline.hpp>
#include <userver/kafka/message.hpp>

#include <kafka/impl/stats.hpp>

#include <librdkafka/rdkafka.h>

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

  /// @brief Schedules the `topics` subscription.
  void Subscribe(const std::vector<std::string>& topics);

  /// @brief Revokes all subscribed topics partitions and leaves the consumer
  /// group.
  /// @note Blocks until consumer successfully closed
  /// @warning Blocks forever if polled messages are not destroyed
  void LeaveGroup();

  /// @brief Closes the consumer and subscribes for the `topics`.
  void Resubscribe(const std::vector<std::string>& topics);

  /// @brief Synchronously commits the current assignment offsets.
  void Commit();

  /// @brief Schedules the committment task.
  void AsyncCommit();

  /// @brief Polls the message until `deadline` is reached.
  /// If no message polled, returns `std::nullopt`
  /// @note Must be called periodically to maintain consumer group membership
  std::optional<Message> PollMessage(engine::Deadline deadline);

  /// @brief Effectively calles `PollMessage` until `deadline` is reached
  /// and no more than `max_batch_size` messages polled.
  MessageBatch PollBatch(std::size_t max_batch_size, engine::Deadline deadline);

  const Stats& GetStats() const;

  void AccountMessageProccessingSucceeded(const Message& message);
  void AccountMessageBatchProccessingSucceeded(const MessageBatch& batch);
  void AccountMessageProccessingFailed(const Message& message);
  void AccountMessageBatchProcessingFailed(const MessageBatch& batch);

  void ErrorCallbackProxy(int error_code, const char* reason);

  /// @brief Callback that is called on each group join/leave and topic
  /// partition update. Used as a dispatcher of rebalance events.
  void RebalanceCallbackProxy(rd_kafka_resp_err_t err,
                              rd_kafka_topic_partition_list_s* partitions);

  /// @brief Assigns (subscribes) the `partitions` list to the current
  /// consumer.
  void AssignPartitions(const rd_kafka_topic_partition_list_s* partitions);

  /// @brief Revokes `partitions` from the current consumer.
  void RevokePartitions(const rd_kafka_topic_partition_list_s* partitions);

  /// @brief Callback which is called after succeeded/failed commit.
  /// Currently, used for logging purposes.
  void OffsetCommitCallbackProxy(
      rd_kafka_resp_err_t err,
      rd_kafka_topic_partition_list_s* committed_offsets);

 private:
  std::shared_ptr<TopicStats> GetTopicStats(const std::string& topic);

  void AccountPolledMessageStat(const Message& polled_message);

 private:
  const std::string component_name_;
  Stats stats_;

  class ConsumerHolder;
  class ConfHolder final {
    using HandleHolder =
        std::unique_ptr<rd_kafka_conf_t, decltype(&rd_kafka_conf_destroy)>;

   public:
    ConfHolder(rd_kafka_conf_t* conf);

    ConfHolder(const ConfHolder&) = delete;
    ConfHolder& operator=(const ConfHolder&) = delete;

    ConfHolder(ConfHolder&&) noexcept = delete;
    ConfHolder& operator=(ConfHolder&&) noexcept = delete;

    HandleHolder MakeConfCopy() const;

   private:
    friend class ConsumerHolder;

    HandleHolder handle_;
  };
  ConfHolder conf_;

  class ConsumerHolder final {
   public:
    ConsumerHolder(ConsumerImpl::ConfHolder::HandleHolder conf_handle);

    ~ConsumerHolder() = default;

    ConsumerHolder(const ConsumerHolder&) = delete;
    ConsumerHolder& operator=(const ConsumerHolder&) = delete;

    ConsumerHolder(ConsumerHolder&&) noexcept = delete;
    ConsumerHolder& operator=(ConsumerHolder&&) noexcept = delete;

    rd_kafka_t* Handle();

   private:
    using HandleHolder =
        std::unique_ptr<rd_kafka_t, decltype(&rd_kafka_destroy)>;
    HandleHolder handle_{nullptr, rd_kafka_destroy};
  };
  std::optional<ConsumerHolder> consumer_;
};

}  // namespace kafka::impl

USERVER_NAMESPACE_END
