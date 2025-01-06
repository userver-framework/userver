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
    const std::string& topic,
    std::uint32_t partition,
    std::optional<std::chrono::milliseconds> timeout
) const {
    return consumer_.GetOffsetRange(topic, partition, timeout);
}

std::vector<std::uint32_t>
ConsumerScope::GetPartitionIds(const std::string& topic, std::optional<std::chrono::milliseconds> timeout) const {
    return consumer_.GetPartitionIds(topic, timeout);
}

}  // namespace kafka

USERVER_NAMESPACE_END
