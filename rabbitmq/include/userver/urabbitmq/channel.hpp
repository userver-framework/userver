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
/// Use this class to publish your messages.
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
/// Use this class to reliably publish your message (publisher-confirms).
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