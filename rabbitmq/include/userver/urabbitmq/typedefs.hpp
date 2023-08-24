#pragma once

/// @file userver/urabbitmq/typedefs.hpp
/// @brief Convenient typedefs for RabbitMQ entities.

#include <userver/utils/strong_typedef.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

/// @brief StrongTypedef alias for a queue name.
class Queue final : public utils::StrongTypedef<class QueueTag, std::string> {
 public:
  using utils::StrongTypedef<QueueTag, std::string>::StrongTypedef;

  /// @brief Queue options, consult RabbitMQ docs for better understanding
  enum class Flags {
    kNone = 0,
    kPassive = 1 << 0,
    kDurable = 1 << 1,
    kExclusive = 1 << 2,
    kAutoDelete = 1 << 3
  };
};

/// @brief StrongTypedef alias for an exchange name.
class Exchange final
    : public utils::StrongTypedef<class ExchangeTag, std::string> {
 public:
  using utils::StrongTypedef<ExchangeTag, std::string>::StrongTypedef;

  /// @brief Type of an exchange.
  ///
  /// Consult RabbitMQ docs for better understanding.
  enum class Type {
    kFanOut,
    kDirect,
    kTopic,
    kHeaders,
    // plugin required
    kConsistentHash,
    // plugin required
    kMessageDeduplication
  };

  /// @brief Exchange options, consult RabbitMQ docs for better understanding
  enum class Flags {
    kNone = 0,
    kPassive = 1 << 0,
    kDurable = 1 << 1,
    kAutoDelete = 1 << 2,
    kInternal = 1 << 3,
    kNoWait = 1 << 4
  };
};

/// @brief Message storage type, consult RabbitMQ docs for better understanding
enum class MessageType {
  kPersistent,
  kTransient,
};

}  // namespace urabbitmq

USERVER_NAMESPACE_END
