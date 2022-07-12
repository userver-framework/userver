#pragma once

#include <memory>
#include <string>

#include <userver/utils/fast_pimpl.hpp>

#include <userver/urabbitmq/exchange_type.hpp>
#include <userver/urabbitmq/typedefs.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

class Client;
class ChannelPtr;

/// Administrative interface for the broker.
/// Use this class to setup your exchanges/queues/bindings.
///
/// Usually retrieved from `Client`
class AdminChannel final {
 public:
  AdminChannel(std::shared_ptr<Client>&& client, ChannelPtr&& channel);
  ~AdminChannel();

  AdminChannel(AdminChannel&& other) noexcept;

  /// Declare an exchange.
  ///
  /// \param exchange name of the exchange
  /// \param type exchange type
  void DeclareExchange(const Exchange& exchange, ExchangeType type);

  /// Declare a queue.
  ///
  /// \param queue name of the queue
  void DeclareQueue(const Queue& queue);

  /// Bind a queue to an exchange.
  ///
  /// \param exchange the source exchange
  /// \param queue the target queue
  /// \param routing_key the routing key
  void BindQueue(const Exchange& exchange, const Queue& queue,
                 const std::string& routing_key);

 private:
  // TODO : this is probably not needed, think about it
  std::shared_ptr<Client> client_;

  class Impl;
  utils::FastPimpl<Impl, 32, 8> impl_;
};

}  // namespace urabbitmq

USERVER_NAMESPACE_END