#pragma once

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

/// Type of an exchange.
///
/// Consult RabbitMQ docs for better understanding.
enum class ExchangeType {
  kFanOut,
  kDirect,
  kTopic,
  kHeaders,
  kConsistentHash,
  kMessageDeduplication
};

}  // namespace urabbitmq

USERVER_NAMESPACE_END
