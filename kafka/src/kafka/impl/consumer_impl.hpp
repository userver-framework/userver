#pragma once

#include <optional>
#include <vector>

#include <librdkafka/rdkafka.h>

#include <userver/engine/deadline.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/kafka/impl/consumer_params.hpp>
#include <userver/kafka/impl/holders.hpp>
#include <userver/kafka/message.hpp>
#include <userver/kafka/offset_range.hpp>
#include <userver/kafka/rebalance_types.hpp>
#include <userver/utils/zstring_view.hpp>

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
    ConsumerImpl(
        const std::string& name,
        const ConfHolder& conf,
        const std::vector<std::string>& topics,
        const ConsumerExecutionParams& execution_params,
        const std::optional<ConsumerRebalanceCallback>& rebalance_callback_opt,
        Stats& stats
    );

    const Stats& GetStats() const;

    /// @brief Synchronously commits the current assignment offsets.
    void Commit();

    /// @brief Schedules the commitment task.
    void AsyncCommit();

    /// @brief Retrieves the low and high offsets for the specified topic and partition.
    OffsetRange GetOffsetRange(
        utils::zstring_view topic,
        std::uint32_t partition,
        std::optional<std::chrono::milliseconds> timeout = std::nullopt
    ) const;

    /// @brief Retrieves the partition IDs for the specified topic.
    std::vector<std::uint32_t>
    GetPartitionIds(utils::zstring_view topic, std::optional<std::chrono::milliseconds> timeout = std::nullopt) const;

    /// @brief Seeks the partition ID for the specified \b topic to a given \b offset .
    void Seek(
        utils::zstring_view topic,
        std::uint32_t partition_id,
        std::uint64_t offset,
        std::chrono::milliseconds timeout
    ) const;

    /// @brief Seeks the partition ID for the specified \b topic to the beginning offset .
    void SeekToBeginning(utils::zstring_view topic, std::uint32_t partition_id, std::chrono::milliseconds timeout)
        const;

    /// @brief Seeks the partition ID for the specified \b topic to the end offset .
    void SeekToEnd(utils::zstring_view topic, std::uint32_t partition_id, std::chrono::milliseconds timeout) const;

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

    /// @brief Schedules the `topics_` subscription.
    void StartConsuming();

private:
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
    void ErrorCallback(rd_kafka_resp_err_t error, const char* reason, bool is_fatal);

    /// @brief Callback called on debug `librdkafka` messages.
    void LogCallback(const char* facility, const char* message, int log_level);

    /// @brief Callback that is called on each group join/leave and topic
    /// partition update. Used as a dispatcher of rebalance events.
    void RebalanceCallback(rd_kafka_resp_err_t err, const rd_kafka_topic_partition_list_s* partitions);

    /// @brief Assigns (subscribes) the `partitions` list to the current
    /// consumer.
    /// @return Returns true if partitions have been successfully assigned.
    bool AssignPartitions(const rd_kafka_topic_partition_list_s* partitions);

    /// @brief Revokes `partitions` from the current consumer.
    /// @return Returns true if partitions have been successfully revoked.
    bool RevokePartitions(const rd_kafka_topic_partition_list_s* partitions);

    /// @brief Calls user's rebalance callback if it is set.
    void CallUserRebalanceCallback(
        const rd_kafka_topic_partition_list_s* partitions,
        RebalanceEventType event_type
    ) noexcept;

    /// @brief Seeks the partition ID for the specified \b topic to a given \b offset .
    void SeekToOffset(
        utils::zstring_view topic,
        std::uint32_t partition_id,
        std::int64_t offset,
        std::chrono::milliseconds timeout
    ) const;

    /// @brief Callback which is called after succeeded/failed commit.
    /// Currently, used for logging purposes.
    void OffsetCommitCallback(rd_kafka_resp_err_t err, const rd_kafka_topic_partition_list_s* committed_offsets);

    std::shared_ptr<TopicStats> GetTopicStats(const std::string& topic);

    void AccountPolledMessageStat(const Message& polled_message);

private:
    const std::string& name_;
    const std::vector<std::string> topics_;
    const ConsumerExecutionParams execution_params_;
    const std::optional<ConsumerRebalanceCallback>& rebalance_callback_;
    Stats& stats_;

    engine::SingleConsumerEvent queue_became_non_empty_event_;

    ConsumerHolder consumer_;
};

}  // namespace kafka::impl

USERVER_NAMESPACE_END
