#include <userver/kafka/exceptions.hpp>

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

}  // namespace kafka

USERVER_NAMESPACE_END
