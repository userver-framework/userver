#include <userver/kafka/exceptions.hpp>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace kafka {

bool SendException::IsRetryable() const noexcept { return is_retryable_; }

SendException::SendException(const char* what, bool is_retryable)
    : std::runtime_error(what), is_retryable_(is_retryable) {}

DeliveryTimeoutException::DeliveryTimeoutException() : SendException(kWhat, /*is_retryable=*/true) {}

QueueFullException::QueueFullException() : SendException(kWhat, /*is_retryable=*/true) {}

MessageTooLargeException::MessageTooLargeException() : SendException(kWhat) {}

UnknownTopicException::UnknownTopicException() : SendException(kWhat) {}

UnknownPartitionException::UnknownPartitionException() : SendException(kWhat) {}

OffsetRangeException::OffsetRangeException(std::string_view what, std::string_view topic, std::uint32_t partition)
    : std::runtime_error(fmt::format("{} topic: '{}', partition: {}", what, topic, partition)) {}

OffsetRangeTimeoutException::OffsetRangeTimeoutException(std::string_view topic, std::uint32_t partition)
    : OffsetRangeException(kWhat, topic, partition) {}

GetMetadataException::GetMetadataException(std::string_view what, std::string_view topic)
    : std::runtime_error(fmt::format("{} topic: '{}'", what, topic)) {}

GetMetadataTimeoutException::GetMetadataTimeoutException(std::string_view topic) : GetMetadataException(kWhat, topic) {}

ParseHeadersException::ParseHeadersException(std::string_view error)
    : std::runtime_error(fmt::format("{}: {}", kWhat, error)) {}

}  // namespace kafka

USERVER_NAMESPACE_END
