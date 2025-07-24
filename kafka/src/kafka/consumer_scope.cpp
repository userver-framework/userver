#include <userver/kafka/consumer_scope.hpp>

#include <userver/kafka/impl/consumer.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka {

ConsumerScope::ConsumerScope(impl::Consumer& consumer) noexcept : consumer_(consumer) {}

ConsumerScope::~ConsumerScope() { Stop(); }

void ConsumerScope::Start(Callback callback) { consumer_.StartMessageProcessing(std::move(callback)); }

void ConsumerScope::Stop() noexcept { consumer_.Stop(); }

void ConsumerScope::AsyncCommit() { consumer_.AsyncCommit(); }

OffsetRange ConsumerScope::GetOffsetRange(
    utils::zstring_view topic,
    std::uint32_t partition,
    std::optional<std::chrono::milliseconds> timeout
) const {
    return consumer_.GetOffsetRange(topic, partition, timeout);
}

std::vector<std::uint32_t>
ConsumerScope::GetPartitionIds(utils::zstring_view topic, std::optional<std::chrono::milliseconds> timeout) const {
    return consumer_.GetPartitionIds(topic, timeout);
}

void ConsumerScope::Seek(
    utils::zstring_view topic,
    std::uint32_t partition_id,
    std::uint64_t offset,
    std::chrono::milliseconds timeout
) const {
    consumer_.Seek(topic, partition_id, offset, timeout);
}

void ConsumerScope::SeekToBeginning(
    utils::zstring_view topic,
    std::uint32_t partition_id,
    std::chrono::milliseconds timeout
) const {
    consumer_.SeekToBeginning(topic, partition_id, timeout);
}

void ConsumerScope::SeekToEnd(utils::zstring_view topic, std::uint32_t partition_id, std::chrono::milliseconds timeout)
    const {
    consumer_.SeekToEnd(topic, partition_id, timeout);
}

void ConsumerScope::SetRebalanceCallback(ConsumerRebalanceCallback rebalance_callback) {
    consumer_.SetRebalanceCallback(std::move(rebalance_callback));
}

void ConsumerScope::ResetRebalanceCallback() { consumer_.ResetRebalanceCallback(); }

}  // namespace kafka

USERVER_NAMESPACE_END
