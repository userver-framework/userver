#pragma once

/// @file userver/urabbitmq/channel.hpp
/// @brief Publisher interface for the broker.

#include <memory>

#include <userver/utils/fast_pimpl.hpp>

#include <userver/urabbitmq/broker_interface.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

class ConnectionPtr;

/// @brief Publisher interface for the broker.
/// You may use this class to publish your messages.
///
/// You may as well use `Client` itself instead, this class just gives you
/// some order guaranties.
///
/// You are not expected to store this class for a long time, because it takes
/// a connection from the underlying connections pool.
///
/// Usually retrieved from `Client`.
class Channel final : IChannelInterface {
 public:
  Channel(ConnectionPtr&& channel);
  ~Channel();

  Channel(Channel&& other) noexcept;

  void Publish(const Exchange& exchange, const std::string& routing_key,
               const std::string& message, MessageType type,
               engine::Deadline deadline) override;

  void Publish(const Exchange& exchange, const std::string& routing_key,
               const std::string& message, engine::Deadline deadline) override {
    Publish(exchange, routing_key, message, MessageType::kTransient, deadline);
  };

 private:
  utils::FastPimpl<ConnectionPtr, 32, 8> impl_;
};

/// @brief Reliable publisher interface for the broker.
/// You may use this class to reliably publish your messages
/// (publisher-confirms).
///
/// You may as well use `Client` itself instead, this class gives no additional
/// benefits and is present merely for convenience.
///
/// You are not expected to store this class for a long time, because it takes
/// a connection from the underlying connections pool.
///
/// Usually retrieved from `Client`.
class ReliableChannel final : IReliableChannelInterface {
 public:
  ReliableChannel(ConnectionPtr&& channel);
  ~ReliableChannel();

  ReliableChannel(ReliableChannel&& other) noexcept;

  void PublishReliable(const Exchange& exchange, const std::string& routing_key,
                       const std::string& message, MessageType type,
                       engine::Deadline deadline) override;

  void PublishReliable(const Exchange& exchange, const std::string& routing_key,
                       const std::string& message,
                       engine::Deadline deadline) override {
    PublishReliable(exchange, routing_key, message, MessageType::kTransient,
                    deadline);
  }

 private:
  utils::FastPimpl<ConnectionPtr, 32, 8> impl_;
};

}  // namespace urabbitmq

USERVER_NAMESPACE_END
