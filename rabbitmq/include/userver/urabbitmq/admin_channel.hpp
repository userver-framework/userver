#pragma once

#include <memory>
#include <string>

#include <userver/urabbitmq/exchange_type.hpp>
#include <userver/urabbitmq/typedefs.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

class Client;
class ChannelPtr;

/// Administrative interface for the broker.
/// Use this class to setup your exchanges/queues/bindings.
///
/// Usually retrieved from `Cluster`
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
  std::shared_ptr<Client> client_;

  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace urabbitmq

USERVER_NAMESPACE_END