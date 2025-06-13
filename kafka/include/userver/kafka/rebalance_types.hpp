
#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include <userver/utils/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka {

enum class RebalanceEventType : int { kAssigned, kRevoked };

/// @brief Represents the topic's partition for certain topic.
struct TopicPartitionView final {
    /// @brief Topic's name.
    std::string_view topic;

    /// @brief Partition ID for a topic
    std::uint32_t partition_id;
};

using TopicPartitionBatchView = utils::span<const TopicPartitionView>;

/// @brief Callback invoked when a rebalance event occurs.
/// @warning The rebalance callback must be set before calling `Start()` or after calling `Stop()`.
/// The callback must not throw exceptions; any thrown exceptions will be caught and logged by the consumer
/// implementation. The callback is invoked after the assign or revoke event has been successfully processed.
using ConsumerRebalanceCallback = std::function<void(TopicPartitionBatchView, RebalanceEventType)>;

}  // namespace kafka

USERVER_NAMESPACE_END
