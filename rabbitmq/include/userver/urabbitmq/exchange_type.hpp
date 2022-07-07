#pragma once

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

enum class ExchangeType {
  kFanOut,
  kDirect,
  kTopic,
  kHeaders,
  kConsistentHash,
  kMessageDeduplication
};

}

USERVER_NAMESPACE_END
