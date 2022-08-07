#pragma once

/// @file userver/urabbitmq/channel.hpp
/// @brief Publisher interface for the broker.

#include <memory>
#include <string>

#include <userver/engine/deadline.hpp>
#include <userver/utils/fast_pimpl.hpp>

#include <userver/urabbitmq/typedefs.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

class ConnectionPtr;

/// @brief Publisher interface for the broker.
/// Use this class to publish your messages.
///
/// Usually retrieved from `Client`.
class Channel final {
 public:
  Channel(ConnectionPtr&& channel);
  ~Channel();

  Channel(Channel&& other) noexcept;

  /// @brief Publish a message to an exchange
  ///
  /// You have to supply the name of the exchange and a routing key. RabbitMQ
  /// will then try to send the message to one or more queues.
  /// By default unroutable messages are silently discarded
  ///
  /// @param exchange the exchange to publish to
  /// @param routing_key the routing key
  /// @param message the message to send
  ///
  /// @note This method is `fire and forget` (no delivery guarantees),
  /// use `ReliableChannel::Publish` for guaranteed delivery.
  void Publish(const Exchange& exchange, const std::string& routing_key,
               const std::string& message,
               MessageType type = MessageType::kTransient);

 private:
  utils::FastPimpl<ConnectionPtr, 32, 8> impl_;
};

/// @brief Reliable publisher interface for the broker.
/// Use this class to reliably publish your message (publisher-confirms).
///
/// Usually retrieved from `Client`.
class ReliableChannel final {
 public:
  ReliableChannel(ConnectionPtr&& channel);
  ~ReliableChannel();

  ReliableChannel(ReliableChannel&& other) noexcept;

  /// @brief Publish a message to an exchange and
  /// await confirmation from the broker
  ///
  /// You have to supply the name of the exchange and a routing key. RabbitMQ
  /// will then try to send the message to one or more queues.
  /// By default unroutable messages are silently discarded
  ///
  /// @param exchange the exchange to publish to
  /// @param routing_key the routing key
  /// @param message the message to send
  /// @param deadline execution deadline
  void Publish(const Exchange& exchange, const std::string& routing_key,
               const std::string& message, MessageType type,
               engine::Deadline deadline);

 private:
  utils::FastPimpl<ConnectionPtr, 32, 8> impl_;
};

}  // namespace urabbitmq

USERVER_NAMESPACE_END