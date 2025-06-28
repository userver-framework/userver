#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include <userver/utils/span.hpp>
#include <userver/utils/zstring_view.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka {

enum class RebalanceEventType { kAssigned, kRevoked };

/// @brief Represents the topic's partition for certain topic.
struct TopicPartitionView final {
    /// @brief Topic's name.
    utils::zstring_view topic;

    /// @brief Partition ID for a topic
    std::uint32_t partition_id;

    /// @brief Offset for current partition if it has commited offset
    std::optional<std::uint64_t> offset;

    TopicPartitionView(utils::zstring_view t, std::uint32_t pid, std::optional<std::uint64_t> off)
        : topic(t), partition_id(pid), offset(off) {}
};

using TopicPartitionBatchView = utils::span<const TopicPartitionView>;

/// @brief Callback invoked when a rebalance event occurs.
/// @warning The rebalance callback must be set before calling ConsumeScope::Start or after calling ConsumeScope::Stop .
/// The callback must not throw exceptions; any thrown exceptions will be caught and logged by the consumer
/// implementation.
/// @note The callback is invoked after the assign or revoke event has been successfully processed.
using ConsumerRebalanceCallback = std::function<void(TopicPartitionBatchView, RebalanceEventType)>;

}  // namespace kafka

USERVER_NAMESPACE_END
