#pragma once

/// @file userver/urabbitmq/client.hpp
/// @brief @copybrief urabbitmq::Client

#include <memory>

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/formats/json_fwd.hpp>
#include <userver/utils/fast_pimpl.hpp>
#include <userver/utils/flags.hpp>

#include <userver/urabbitmq/client_settings.hpp>
#include <userver/urabbitmq/typedefs.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

class AdminChannel;
class Channel;
class ReliableChannel;
class ConsumerBase;
class ClientImpl;

/// @ingroup userver_clients
///
/// @brief Interface for communicating with a RabbitMQ cluster.
///
/// Usually retrieved from components::RabbitMQ component.
class Client : public std::enable_shared_from_this<Client> {
 public:
  /// Client factory function
  /// @param resolver asynchronous DNS resolver
  /// @param settings client settings
  static std::shared_ptr<Client> Create(clients::dns::Resolver& resolver,
                                        const ClientSettings& settings);
  /// Client destructor
  ~Client();

  /// @brief Declare an exchange.
  ///
  /// @param exchange name of the exchange
  /// @param type exchange type
  /// @param flags exchange flags
  /// @param deadline execution deadline
  void DeclareExchange(const Exchange& exchange, Exchange::Type type,
                       utils::Flags<Exchange::Flags> flags,
                       engine::Deadline deadline);

  /// @brief overload of DeclareExchange
  void DeclareExchange(const Exchange& exchange, Exchange::Type type,
                       engine::Deadline deadline) {
    DeclareExchange(exchange, type, {}, deadline);
  }

  /// @brief overload of DeclareExchange
  void DeclareExchange(const Exchange& exchange, engine::Deadline deadline) {
    DeclareExchange(exchange, Exchange::Type::kFanOut, {}, deadline);
  }

  /// @brief Declare a queue.
  ///
  /// @param queue name of the queue
  /// @param flags queue flags
  /// @param deadline execution deadline
  void DeclareQueue(const Queue& queue, utils::Flags<Queue::Flags> flags,
                    engine::Deadline deadline);

  /// @brief overload of DeclareQueue
  void DeclareQueue(const Queue& queue, engine::Deadline deadline) {
    DeclareQueue(queue, {}, deadline);
  }

  /// @brief Bind a queue to an exchange.
  ///
  /// @param exchange the source exchange
  /// @param queue the target queue
  /// @param routing_key the routing key
  /// @param deadline execution deadline
  void BindQueue(const Exchange& exchange, const Queue& queue,
                 const std::string& routing_key, engine::Deadline deadline);

  /// @brief Remove an exchange.
  ///
  /// @param exchange name of the exchange to remove
  /// @param deadline execution deadline
  void RemoveExchange(const Exchange& exchange, engine::Deadline deadline);

  /// @brief Remove a queue.
  ///
  /// @param queue name of the queue to remove
  /// @param deadline execution deadline
  void RemoveQueue(const Queue& queue, engine::Deadline deadline);

  /// @brief Get an administrative interface for the broker.
  AdminChannel GetAdminChannel();

  /// @brief Publish a message to an exchange
  ///
  /// You have to supply the name of the exchange and a routing key. RabbitMQ
  /// will then try to send the message to one or more queues.
  /// By default unroutable messages are silently discarded
  ///
  /// @param exchange the exchange to publish to
  /// @param routing_key the routing key
  /// @param message the message to send
  /// @param deadline execution deadline
  ///
  /// @note This method is `fire and forget` (no delivery guarantees),
  /// use `PublishReliable` for delivery guarantees.
  void Publish(const Exchange& exchange, const std::string& routing_key,
               const std::string& message, MessageType type,
               engine::Deadline deadline);

  /// @brief Get a publisher interface for the broker.
  Channel GetChannel();

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
  void PublishReliable(const Exchange& exchange, const std::string& routing_key,
                       const std::string& message, MessageType type,
                       engine::Deadline deadline);

  /// @brief Get a reliable publisher interface for the broker
  /// (publisher-confirms)
  ReliableChannel GetReliableChannel();

  /// Get cluster statistics
  formats::json::Value GetStatistics() const;

 protected:
  Client(clients::dns::Resolver& resolver, const ClientSettings& settings);

 private:
  friend class ConsumerBase;
  utils::FastPimpl<ClientImpl, 232, 8> impl_;
};

}  // namespace urabbitmq

USERVER_NAMESPACE_END
