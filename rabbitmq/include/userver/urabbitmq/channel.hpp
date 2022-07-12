#pragma once

#include <memory>
#include <string>

#include <userver/utils/fast_pimpl.hpp>

#include <userver/urabbitmq/typedefs.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

class Client;
class ChannelPtr;

/// Publisher interface for the broker. Use this class to publish your message.
///
/// Usually retrieved from `Client`.
class Channel final {
 public:
  Channel(std::shared_ptr<Client>&& client, ChannelPtr&& channel,
          ChannelPtr&& reliable_channel);
  ~Channel();

  Channel(Channel&& other) noexcept;

  /// Publish a message to an exchange
  ///
  /// You have to supply the name of the exchange and a routing key. RabbitMQ
  /// will then try to send the message to one or more queues.
  /// By default unroutable messages are silently discarded
  ///
  /// \param exchange the exchange to publish to
  /// \param routing_key the routing key
  /// \param message the message to send
  ///
  /// \note This method if `fire and forget` (no delivery guarantees),
  /// use `PublishReliable` for guaranteed delivery.
  void Publish(const Exchange& exchange, const std::string& routing_key,
               const std::string& message);

  /// Same as `Publish`, but also awaits broker confirmation.
  ///
  /// Use this method instead of `Publish` if you need delivery guarantees:
  /// retrying on failures guarantees `at least once` delivery.
  void PublishReliable(const Exchange& exchange, const std::string& routing_key,
                       const std::string& message);

 private:
  // TODO : this is probably not needed, think about it
  std::shared_ptr<Client> client_;

  class Impl;
  utils::FastPimpl<Impl, 64, 8> impl_;
};

}  // namespace urabbitmq

USERVER_NAMESPACE_END